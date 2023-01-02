/*
 * (C) 2011-2023 by Christian Hesse <mail@eworm.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "udev-block-notify.h"
#include "config.h"
#include "version.h"

const static char optstring[] = "ht:vV";
const static struct option options_long[] = {
	/* name		has_arg			flag	val */
	{ "help",	no_argument,		NULL,	'h' },
	{ "timeout",	required_argument,	NULL,	't' },
	{ "verbose",	no_argument,		NULL,	'v' },
	{ "version",	no_argument,		NULL,	'V' },
	{ 0, 0, 0, 0 }
};

char *program;
uint8_t verbose = 0;
uint8_t doexit = 0;

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

/*** received_signal ***/
void received_signal(int signal) {
	if (verbose > 0)
		printf("%s: Received signal: %s\n", program, strsignal(signal));

	doexit++;
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
	unsigned int notification_timeout = NOTIFICATION_TIMEOUT;

	int i;
	dev_t devnum = 0;
	unsigned int major = 0, minor = 0;
	struct udev_device *dev = NULL;
	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;

	unsigned int version = 0, help = 0;

	program = argv[0];

	/* get the verbose status */
	while ((i = getopt_long(argc, argv, optstring, options_long, NULL)) != -1) {
		switch (i) {
			case 'h':
				help++;
				break;
			case 't':
				notification_timeout = atof(optarg) * 1000;
				break;
			case 'v':
				verbose++;
				break;
			case 'V':
				verbose++;
				version++;
				break;
		}
	}

	if (verbose > 0)
		printf("%s: %s v%s"
#ifdef HAVE_SYSTEMD
			" +systemd"
#endif
			" (compiled: " __DATE__ ", " __TIME__ ")\n", program, PROGNAME, VERSION);

	if (help > 0)
		printf("usage: %s [-h] [-t TIMEOUT] [-v] [-V]\n", program);

	if (version > 0 || help > 0)
		return EXIT_SUCCESS;

	if(notify_init("Udev-Block-Notification") == FALSE) {
		fprintf(stderr, "%s: Can't create notify.\n", program);
		exit(EXIT_FAILURE);
	}

	if ((udev = udev_new()) == NULL) {
		fprintf(stderr, "%s: Can't create udev.\n", program);
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

	signal(SIGINT, received_signal);
	signal(SIGTERM, received_signal);

#ifdef HAVE_SYSTEMD
	sd_notify(0, "READY=1\nSTATUS=Waiting for udev block events...");
#endif

	while (doexit == 0) {
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
							printf("%s: Skipping temporary DM device %s\n", program, device);
						continue;
					}
					if ((property = udev_device_get_property_value(dev, "DM_LV_LAYER")) != NULL) {
						if (strcmp(property, "cow") == 0 || strcmp(property, "real") == 0) {
							if (verbose > 0)
								printf("%s: Skipping DM %s device %s\n", program, property, device);
							continue;
						}
					}
				}

				devnum = udev_device_get_devnum(dev);
				major = major(devnum);
				minor = minor(devnum);

				if (verbose > 0)
					printf("%s: Processing device %d:%d\n", program, major, minor);

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
					printf("%s: %s\n", program, notifystr);

				/* get a notification */
				notification = get_notification(notifications, devnum);

				/* this is a fallback and should be replaced below
				 * if it is not... what drive is it? */
				icon = ICON_UNKNOWN;

				/* decide about what icon to use */
				value = udev_device_get_property_value(dev, "ID_BUS");
				if (udev_device_get_property_value(dev, "ID_CDROM") != NULL) { /* optical drive */
					if (udev_device_get_property_value(dev, "ID_CDROM_MEDIA_TRACK_COUNT_AUDIO") != NULL) {
						icon = ICON_MEDIA_OPTICAL_CD_AUDIO;
					} else {
						icon = ICON_DRIVE_OPTICAL;
					}
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
				notify_notification_set_timeout(notification, notification_timeout);

				if (notify_notification_show(notification, &error) == FALSE) {
					g_printerr("%s: Error showing notification: %s\n", program, error->message);
					g_error_free(error);

					exit(EXIT_FAILURE);
				}

				free(notifystr);
				udev_device_unref(dev);
			}

			// This is not really needed... But we want to make shure not to eat 100% CPU if anything breaks. ;)
			usleep(50 * 1000);
		}
	}

	/* report stopping to systemd */
#ifdef HAVE_SYSTEMD
	sd_notify(0, "STOPPING=1\nSTATUS=Stopping...");
#endif

	udev_unref(udev);

#ifdef HAVE_SYSTEMD
	sd_notify(0, "STATUS=Stopped. Bye!");
#endif

	return EXIT_SUCCESS;
}
