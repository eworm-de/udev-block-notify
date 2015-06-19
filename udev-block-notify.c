/*
 * (C) 2011-2015 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include "udev-block-notify.h"
#include "config.h"
#include "version.h"

const static char optstring[] = "hv";
const static struct option options_long[] = {
	/* name		has_arg		flag	val */
	{ "help",	no_argument,	NULL,	'h' },
	{ "verbose",	no_argument,	NULL,	'v' },
	{ 0, 0, 0, 0 }
};

uint8_t verbose = 0;

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
char * newstr(const char *text, const char *device, unsigned short int major, unsigned short int minor) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(device) + 10 /* max string length 2* unsigned short int */);
	sprintf(notifystr, text, device, major, minor);

	return notifystr;
}

/*** appendstr ***/
char * appendstr(const char *text, char *notifystr, const char *property, const char *value) {
	notifystr = realloc(notifystr, strlen(text) + strlen(notifystr) + strlen(property) + strlen(value));
	sprintf(notifystr + strlen(notifystr), text, property, value);

	return notifystr;
}

/*** main ***/
int main (int argc, char ** argv) {
	const char *action = NULL;
	const char *device = NULL, *value = NULL;
	char *icon = NULL, *notifystr = NULL;
	fd_set readfds;
	GError *error = NULL;
	NotifyNotification *notification = NULL;
	struct notifications *notifications = NULL;
	int errcount = 0, i;
	dev_t devnum = 0;
	unsigned int major = 0, minor = 0;
	struct udev_device *dev = NULL;
	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;

	/* get the verbose status */
	while ((i = getopt_long(argc, argv, optstring, options_long, NULL)) != -1) {
		switch (i) {
			case 'h':
				printf("usage: %s [-h] [-v]\n", argv[0]);
				return EXIT_SUCCESS;
			case 'v':
				verbose++;
				break;
		}
	}

	printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__ ")\n", argv[0], PROGNAME, VERSION);

	if(notify_init("Udev-Block-Notification") == FALSE) {
		fprintf(stderr, "%s: Can't create notify.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((udev = udev_new()) == NULL) {
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
				device = udev_device_get_sysname(dev);

				/* ignore temporary device mapper devices
				 * is there a better way to do this? */
				if (strncmp(device, "dm", 2) == 0) {
					const char * property;

					if (udev_device_get_property_value(dev, "DM_NAME") == NULL) {
						if (verbose > 0)
							printf("%s: Skipping temporary DM device %s\n", argv[0], device);
						continue;
					}
					if ((property = udev_device_get_property_value(dev, "DM_LV_LAYER")) != NULL) {
						if (strcmp(property, "cow") == 0 || strcmp(property, "real") == 0) {
							if (verbose > 0)
								printf("%s: Skipping DM %s device %s\n", argv[0], property, device);
							continue;
						}
					}
				}

				devnum = udev_device_get_devnum(dev);
				major = major(devnum);
				minor = minor(devnum);

				if (verbose > 0)
					printf("%s: Processing device %d:%d\n", argv[0], major, minor);

				action = udev_device_get_action(dev);
				if (strcmp(action, "add") == 0)
					notifystr = newstr(TEXT_ADD, device, major, minor);
				else if (strcmp(action, "remove") == 0)
					notifystr = newstr(TEXT_REMOVE, device, major, minor);
				else if (strcmp(action, "move") == 0)
					notifystr = newstr(TEXT_MOVE, device, major, minor);
				else if (strcmp(action, "change") == 0)
					notifystr = newstr(TEXT_CHANGE, device, major, minor);
				else
					/* we should never get here I think... */
					notifystr = newstr(TEXT_DEFAULT, device, major, minor);

				/* Get possible values with:
				 * $ udevadm info --query=all --name=/path/to/dev
				 * Values available differs from device type and content */

				/* file system */
				if ((value = udev_device_get_property_value(dev, "ID_FS_LABEL")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Label", value);
				if ((value = udev_device_get_property_value(dev, "ID_FS_TYPE")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Type", value);
				if ((value = udev_device_get_property_value(dev, "ID_FS_USAGE")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Usage", value);
				if ((value = udev_device_get_property_value(dev, "ID_FS_UUID")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "UUID", value);

				/* partition */
				if ((value = udev_device_get_property_value(dev, "ID_PART_TABLE_TYPE")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Partition Table Type", value);
				if ((value = udev_device_get_property_value(dev, "ID_PART_TABLE_NAME")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Partition Name", value);
				if ((value = udev_device_get_property_value(dev, "ID_PART_ENTRY_TYPE")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Partition Type", value);

				/* device mapper */
				if ((value = udev_device_get_property_value(dev, "DM_NAME")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Device mapper name", value);

				/* multi disk */
				if ((value = udev_device_get_property_value(dev, "MD_LEVEL")) != NULL && *value != 0)
					notifystr = appendstr(TEXT_TAG, notifystr, "Multi disk level", value);

				if (verbose > 0)
					printf("%s: %s\n", argv[0], notifystr);

				/* get a notification */
				notification = get_notification(notifications, devnum);

				/* this is a fallback and should be replaced below
				 * if it is not... what drive is it? */
				icon = ICON_UNKNOWN;

				/* decide about what icon to use */
				value = udev_device_get_property_value(dev, "ID_BUS");
				if (udev_device_get_property_value(dev, "ID_CDROM") != NULL) { /* optical drive */
					icon = ICON_DRIVE_OPTICAL;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLOPPY") != NULL) { /* floppy drive */
					icon = ICON_MEDIA_FLOPPY;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_THUMB") != NULL) { /* thumb drive, e.g. USB flash */
					icon = ICON_MEDIA_REMOVABLE;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLASH_CF") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_MS") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_SD") != NULL ||
						udev_device_get_property_value(dev, "ID_DRIVE_FLASH_SM") != NULL) { /* flash card reader */
							/* note that usb card reader are recognized as USB hard disk */
					icon = ICON_MEDIA_FLASH;
				} else if (udev_device_get_property_value(dev, "ID_DRIVE_FLOPPY_ZIP") != NULL) { /* ZIP drive */
					icon = ICON_MEDIA_ZIP;
				} else if (udev_device_get_property_value(dev, "ID_MEDIA_PLAYER") != NULL) { /* media player */
					icon = ICON_MULTIMEDIA_PLAYER;
				} else if (udev_device_get_property_value(dev, "DM_NAME") != NULL) { /* device mapper */
					icon = ICON_DEVICE_MAPPER;
				} else if (udev_device_get_property_value(dev, "MD_NAME") != NULL) { /* multi disk */
					icon = ICON_DRIVE_MULTIDISK;
				} else if (strncmp(device, "loop", 4) == 0 ||
						strncmp(device, "ram", 3) == 0) { /* loop & RAM */
					icon = ICON_LOOP;
				} else if (strncmp(device, "nbd", 3) == 0) { /* network block device */
					icon = ICON_NETWORK_SERVER;
				} else if (value != NULL) {
					if (strcmp(value, "ata") == 0 ||
							strcmp(value, "scsi") == 0) { /* internal (s)ata/scsi hard disk */
						icon = ICON_DRIVE_HARDDISK;
					} else if (strcmp(value, "usb") == 0) { /* USB hard disk */
						icon = ICON_DRIVE_HARDDISK_USB;
					} else if (strcmp(value, "ieee1394") == 0) { /* firewire hard disk */
						icon = ICON_DRIVE_HARDDISK_IEEE1394;
					}
				}

				notify_notification_update(notification, TEXT_TOPIC, notifystr, icon);
				notify_notification_set_timeout(notification, NOTIFICATION_TIMEOUT);

				while(notify_notification_show(notification, &error) == FALSE) {
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

						if(notify_init("Udev-Block-Notification") == FALSE) {
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
