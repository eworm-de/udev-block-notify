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

#define ICON_ADD	"media-removable"
#define ICON_REMOVE	"media-removable"
#define ICON_MOVE	"media-removable"
#define ICON_CHANGE	"media-removable"
#define ICON_DEFAULT	"media-removable"

#define TEXT_TOPIC	"Udev Block Notification"
#define TEXT_ADD	"Device <b>%s</b> (%i:%i) <b>appeared</b>."
#define TEXT_REMOVE	"Device <b>%s</b> (%i:%i) <b>disappeared</b>."
#define TEXT_MOVE	"Device <b>%s</b> (%i:%i) was <b>renamed</b>."
#define TEXT_CHANGE	"Device <b>%s</b> (%i:%i) media <b>changed</b>."
#define TEXT_DEFAULT	"Anything happend to <b>%s</b> (%i:%i)... Don't know."
#define TEXT_TAG	"\n%s: <i>%s</i>"

/* newstr */
char * newstr(char *text, char *device, unsigned short int major, unsigned short int minor) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(device) + 10 /* max string length 2* unsigned short int */);
	sprintf(notifystr, text, device, major, minor);

	return notifystr;
}

/* appendstr */
char * appendstr(char *text, char *notifystr, char *property, const char *value) {
	notifystr = realloc(notifystr, strlen(text) + strlen(notifystr) + strlen(property) + strlen(value));
	sprintf(notifystr + strlen(notifystr), text, property, value);

	return notifystr;
}

/* main */
int main (int argc, char ** argv) {
	char action;
	char *device = NULL, *icon = NULL, *notifystr = NULL;
	const char *value = NULL;
	fd_set readfds;
	GError *error = NULL;
	NotifyNotification ***notification = NULL;
	int errcount = 0;
	dev_t devnum = 0;
	unsigned int major = 0, minor = 0, maxmajor = 0;
	unsigned int *maxminor = NULL;
	struct udev_device *dev = NULL;
	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;

	printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__
#if DEBUG
			", with debug output"
#endif
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
#if DEBUG
				printf("%s: Processing device %d:%d\n", argv[0], major, minor);
#endif
				/* make sure we have allocated memory */
				if (maxmajor < major) {
					notification = realloc(notification, (major + 1) * sizeof(size_t));
					maxminor = realloc(maxminor, (major + 1) * sizeof(unsigned int));
					while (maxmajor <= major) {
						notification[maxmajor] = NULL;
						maxminor[maxmajor] = 0;
						maxmajor++;
					}
					maxmajor--;
				}
				if (maxminor[major] < minor) {
					notification[major] = realloc(notification[major], (minor + 1) * sizeof(size_t));
					while (maxminor[major] <= minor) {
						notification[major][maxminor[major]] = NULL;
						maxminor[major]++;
					}
					maxminor[major]--;
				}

				action = udev_device_get_action(dev)[0];
				switch(action) {
					case 'a':
						// a: add
						notifystr = newstr(TEXT_ADD, device, major, minor);
						icon = ICON_ADD;
						break;
					case 'r':
						// r: remove
						notifystr = newstr(TEXT_REMOVE, device, major, minor);
						icon = ICON_REMOVE;
						break;
					case 'm':
						// m: move
						notifystr = newstr(TEXT_MOVE, device, major, minor);
						icon = ICON_MOVE;
						break;
					case 'c':
						// c: change
						notifystr = newstr(TEXT_CHANGE, device, major, minor);
						icon = ICON_CHANGE;
						break;
					default:
						// we should never get here I think...
						notifystr = newstr(TEXT_DEFAULT, device, major, minor);
						icon = ICON_DEFAULT;
				}

				if (action != 'r') {
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

#if DEBUG
				printf("%s: %s\n", argv[0], notifystr);
#endif

				if (notification[major][minor] == NULL) {
					notification[major][minor] = notify_notification_new(TEXT_TOPIC, notifystr, icon);
					notify_notification_set_category(notification[major][minor], PROGNAME);
					notify_notification_set_urgency (notification[major][minor], NOTIFY_URGENCY_NORMAL);
				} else
					notify_notification_update(notification[major][minor], TEXT_TOPIC, notifystr, icon);

				notify_notification_set_timeout(notification[major][minor], NOTIFICATION_TIMEOUT);

				while(!notify_notification_show(notification[major][minor], &error)) {
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
