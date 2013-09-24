/*
 * (C) 2011-2013 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libnotify/notify.h>
#include <libudev.h>

#define PROGNAME	"udev-block-notify"

#define NOTIFICATION_TIMEOUT	10000
#ifndef DEBUG
#define DEBUG	0
#endif

#define ICON_DEVICE_MAPPER		"media-playlist-shuffle"
#define ICON_DRIVE_HARDDISK		"drive-harddisk"
#define ICON_DRIVE_HARDDISK_IEEE1394	"drive-harddisk-ieee1394"
#define ICON_DRIVE_HARDDISK_USB		"drive-harddisk-usb"
#define ICON_DRIVE_OPTICAL		"drive-optical"
#define ICON_LOOP			"media-playlist-repeat"
#define ICON_MEDIA_FLASH		"media-flash"
#define ICON_MEDIA_FLOPPY		"media-floppy"
#define ICON_MEDIA_REMOVABLE		"media-removable"
#define ICON_MEDIA_ZIP			"media-zip"
#define ICON_MULTIMEDIA_PLAYER		"multimedia-player"
#define ICON_UNKNOWN			"dialog-question"

#define TEXT_TOPIC	"Udev Block Notification"
#define TEXT_ADD	"Device <b>%s</b> (%i:%i) <b>appeared</b>."
#define TEXT_REMOVE	"Device <b>%s</b> (%i:%i) <b>disappeared</b>."
#define TEXT_MOVE	"Device <b>%s</b> (%i:%i) was <b>renamed</b>."
#define TEXT_CHANGE	"Device <b>%s</b> (%i:%i) media <b>changed</b>."
#define TEXT_DEFAULT	"Anything happend to <b>%s</b> (%i:%i)... Don't know."
#define TEXT_TAG	"\n%s: <i>%s</i>"

struct notifications {
	dev_t devnum;
	NotifyNotification *notification;
	struct notifications *next;
};

/*** get_notification ***/
NotifyNotification * get_notification(struct notifications *notifications, dev_t devnum) {
	/* work with notifications->next here, we need it later to not miss
	 * the pointer in main struct */
	while (notifications->next != NULL) {
		if (notifications->next->devnum == devnum)
			return notifications->next->notification;
		notifications = notifications->next;
	}

	/* did not find the notification, creating a new struct element */
	notifications->next = malloc(sizeof(struct notifications));
	notifications = notifications->next;
	notifications->devnum = devnum;
	notifications->notification =
#		if NOTIFY_CHECK_VERSION(0, 7, 0)
		notify_notification_new(NULL, NULL, NULL);
#		else
		notify_notification_new(NULL, NULL, NULL, NULL);
#		endif
	notifications->next = NULL;

	notify_notification_set_category(notifications->notification, PROGNAME);
	notify_notification_set_urgency (notifications->notification, NOTIFY_URGENCY_NORMAL);

	return notifications->notification;
}

/*** newstr ***/
char * newstr(const char *text, char *device, unsigned short int major, unsigned short int minor) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(device) + 10 /* max string length 2* unsigned short int */);
	sprintf(notifystr, text, device, major, minor);

	return notifystr;
}

/*** appendstr ***/
char * appendstr(const char *text, char *notifystr, char *property, const char *value) {
	notifystr = realloc(notifystr, strlen(text) + strlen(notifystr) + strlen(property) + strlen(value));
	sprintf(notifystr + strlen(notifystr), text, property, value);

	return notifystr;
}

/*** main ***/
int main (int argc, char ** argv) {
	char action;
	char *device = NULL, *icon = NULL, *notifystr = NULL;
	const char *value = NULL;
	fd_set readfds;
	GError *error = NULL;
	NotifyNotification *notification = NULL;
	struct notifications *notifications = NULL;
	int errcount = 0;
	dev_t devnum = 0;
	unsigned int major = 0, minor = 0;
	struct udev_device *dev = NULL;
	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;

	printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__
#			if DEBUG
			", with debug output"
#			endif
	")\n", argv[0], PROGNAME, VERSION);

	if(!notify_init("Udev-Block-Notification")) {
		fprintf(stderr, "%s: Can't create notify.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	udev = udev_new();
	if(!udev) {
		fprintf(stderr, "%s: Can't create udev.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
	udev_monitor_enable_receiving(mon);

	/* allocate first struct element as dummy */
	notifications = malloc(sizeof(struct notifications));
	notifications->devnum = 0;
	notifications->notification = NULL;
	notifications->next = NULL;

	while (1) {
		FD_ZERO(&readfds);
		if (mon != NULL)
			FD_SET(udev_monitor_get_fd(mon), &readfds);

		select(udev_monitor_get_fd(mon) + 1, &readfds, NULL, NULL, NULL);

		if ((mon != NULL) && FD_ISSET(udev_monitor_get_fd(mon), &readfds)) {
			dev = udev_monitor_receive_device(mon);
			if(dev) {
				device = (char *) udev_device_get_sysname(dev);
				devnum = udev_device_get_devnum(dev);
				major = major(devnum);
				minor = minor(devnum);
#				if DEBUG
				printf("%s: Processing device %d:%d\n", argv[0], major, minor);
#				endif
				action = udev_device_get_action(dev)[0];
				switch(action) {
					case 'a':
						// a: add
						notifystr = newstr(TEXT_ADD, device, major, minor);
						break;
					case 'r':
						// r: remove
						notifystr = newstr(TEXT_REMOVE, device, major, minor);
						break;
					case 'm':
						// m: move
						notifystr = newstr(TEXT_MOVE, device, major, minor);
						break;
					case 'c':
						// c: change
						notifystr = newstr(TEXT_CHANGE, device, major, minor);
						break;
					default:
						// we should never get here I think...
						notifystr = newstr(TEXT_DEFAULT, device, major, minor);
				}

				if (action != 'r') {
					/* Get possible values with:
					 * $ udevadm info --query=all --name=/path/to/dev
					 * Values available differs from device type and content */
					if ((value = udev_device_get_property_value(dev, "ID_FS_LABEL")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Label", value);
					if ((value = udev_device_get_property_value(dev, "ID_FS_TYPE")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Type", value);
					if ((value = udev_device_get_property_value(dev, "ID_FS_USAGE")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Usage", value);
					if ((value = udev_device_get_property_value(dev, "ID_FS_UUID")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "UUID", value);
					if ((value = udev_device_get_property_value(dev, "ID_PART_TABLE_TYPE")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Partition Table Type", value);
					if ((value = udev_device_get_property_value(dev, "ID_PART_TABLE_NAME")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Partition Name", value);
					if ((value = udev_device_get_property_value(dev, "ID_PART_ENTRY_TYPE")) != NULL)
						notifystr = appendstr(TEXT_TAG, notifystr, "Partition Type", value);
				}

#				if DEBUG
				printf("%s: %s\n", argv[0], notifystr);
#				endif

				/* get a notification */
				notification = get_notification(notifications, devnum);

				/* decide about what icon to use */
				value = udev_device_get_property_value(dev, "ID_BUS");
				if (udev_device_get_property_value(dev, "ID_DRIVE_CDROM") != NULL) {
					icon = ICON_DRIVE_OPTICAL;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLOPPY") != NULL) {
					icon = ICON_MEDIA_FLOPPY;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_THUMB") != NULL) {
					icon = ICON_MEDIA_REMOVABLE;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLASH_CF") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_MS") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_SD") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_SM") != NULL) {
					icon = ICON_MEDIA_FLASH;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLOPPY_ZIP") != NULL) {
					icon = ICON_MEDIA_ZIP;
				} else if (udev_device_get_property_value(dev, "ID_MEDIA_PLAYER") != NULL) {
					icon = ICON_MULTIMEDIA_PLAYER;
				} else if (udev_device_get_property_value(dev, "DM_NAME") != NULL) {
					icon = ICON_DEVICE_MAPPER;
				} else if (strncmp(device, "loop", 4) == 0 ||
						strncmp(device, "ram", 3) == 0) {
					icon = ICON_LOOP;
				} else if (value != NULL) {
					if (strcmp(value, "ata") == 0 ||
							strcmp(value, "scsi") == 0) {
						icon = ICON_DRIVE_HARDDISK;
					} else if (strcmp(value, "usb") == 0) {
						icon = ICON_DRIVE_HARDDISK_USB;
					} else if (strcmp(value, "ieee1394") == 0) {
						icon = ICON_DRIVE_HARDDISK_IEEE1394;
					}
				} else
					/* we should never get here... what drive is it? */
					icon = ICON_UNKNOWN;

				notify_notification_update(notification, TEXT_TOPIC, notifystr, icon);
				notify_notification_set_timeout(notification, NOTIFICATION_TIMEOUT);

				while(!notify_notification_show(notification, &error)) {
					if (errcount > 1) {
						fprintf(stderr, "%s: Looks like we can not reconnect to notification daemon... Exiting.\n", argv[0]);
						exit(EXIT_FAILURE);
					} else {
						g_printerr("%s: Error \"%s\" while trying to show notification. Trying to reconnect.\n", argv[0], error->message);
						errcount++;

						g_error_free(error);
						error = NULL;

						notify_uninit();

						usleep(500 * 1000);

						if(!notify_init("Udev-Block-Notification")) {
							fprintf(stderr, "%s: Can't create notify.\n", argv[0]);
							exit(EXIT_FAILURE);
						}
					}
				}
				errcount = 0;

				free(notifystr);
				udev_device_unref(dev);
			}

			// This is not really needed... But we want to make shure not to eat 100% CPU if anything breaks. ;)
			usleep(50 * 1000);
		}
	}

	udev_unref(udev);
	return EXIT_SUCCESS;
}
