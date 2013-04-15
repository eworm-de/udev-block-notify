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
#include <blkid.h>

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
#define TEXT_TAG	"%s\n%s: <i>%s</i>"

int main (int argc, char ** argv) {
	blkid_cache cache = NULL;
	blkid_dev blkdev = NULL;
	blkid_tag_iterate iter = NULL;
	char action;
	char *device = NULL, *icon = NULL, *notifystr = NULL, *read = NULL;
	const char *type, *value, *devname;
	fd_set readfds;
	GError *error = NULL;
	int fdcount, devnum, errcount = 0;
        NotifyNotification *notification;
        NotifyNotification ***notificationref;
	struct udev_device *dev = NULL;
   	struct udev_monitor *mon = NULL;
	struct udev *udev = NULL;
	short int *maxminor;
	unsigned short int i, major, minor;

	printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__ ")\n", argv[0], PROGNAME, VERSION);

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

	notificationref = malloc(256 * sizeof(size_t));
	for(i = 0; i < 256; i++)
		notificationref[i] = NULL;
	maxminor = malloc(256 * sizeof(short int));
	for(i = 0; i < 256; i++)
		maxminor[i] = -1;

	while (1) {
                FD_ZERO(&readfds);
                if (mon != NULL)
                        FD_SET(udev_monitor_get_fd(mon), &readfds);

                fdcount = select(udev_monitor_get_fd(mon) + 1, &readfds, NULL, NULL, NULL);

                if ((mon != NULL) && FD_ISSET(udev_monitor_get_fd(mon), &readfds)) {
			dev = udev_monitor_receive_device(mon);
			if(dev) {
				device = (char *) udev_device_get_sysname(dev);
				devnum = udev_device_get_devnum(dev);
				major = devnum / 256;
				minor = devnum - (major * 256);

				action = udev_device_get_action(dev)[0];
				switch(action) {
					case 'a':
						// a: add
						notifystr = (char *) malloc(strlen(TEXT_ADD) + strlen(device));
						sprintf(notifystr, TEXT_ADD, device, major, minor);
						icon = ICON_ADD;
						break;
					case 'r':
						// r: remove
						notifystr = (char *) malloc(strlen(TEXT_REMOVE) + strlen(device));
						sprintf(notifystr, TEXT_REMOVE, device, major, minor);
						icon = ICON_REMOVE;
						break;
					case 'm':
						// m: move
						notifystr = (char *) malloc(strlen(TEXT_MOVE) + strlen(device));
						sprintf(notifystr, TEXT_MOVE, device, major, minor);
						icon = ICON_MOVE;
						break;
					case 'c':
						// c: change
						notifystr = (char *) malloc(strlen(TEXT_CHANGE) + strlen(device));
						sprintf(notifystr, TEXT_CHANGE, device, major, minor);
						icon = ICON_CHANGE;
						break;
					default:
						// we should never get here I think...
						notifystr = (char *) malloc(strlen(TEXT_DEFAULT) + strlen(device));
						sprintf(notifystr, TEXT_CHANGE, device, major, minor);
						icon = ICON_DEFAULT;
				}

				if (blkid_get_cache(&cache, read) != 0)
					fprintf(stderr, "%s: Could not get blkid cache.\n", argv[0]);

				if (blkid_probe_all_new(cache) != 0)
					fprintf(stderr, "%s: Could not probe new devices.\n", argv[0]);

				if (action != 'r') {
					blkdev = blkid_get_dev(cache, udev_device_get_devnode(dev), BLKID_DEV_NORMAL);
			
					if (blkdev) {
						iter = blkid_tag_iterate_begin(blkdev);
				
						while (blkid_tag_next(iter, &type, &value) == 0) {
							notifystr = (char *) realloc(notifystr, strlen(TEXT_TAG) + strlen(notifystr) + strlen(type) + strlen(value));
							sprintf(notifystr, TEXT_TAG, notifystr, type, value);
						}
	
						blkid_tag_iterate_end(iter);
						blkid_put_cache(cache);
					} else
						fprintf(stderr, "%s: Could not get blkid device.\n", argv[0]);
				}

#if DEBUG
				printf("%s: %s\n", argv[0], notifystr);
#endif

				if (maxminor[major] < minor) {
					notificationref[major] = realloc(notificationref[major], (minor + 1) * sizeof(size_t));
			                while(maxminor[major] < minor)
			                        notificationref[major][++maxminor[major]] = NULL;
			        }
					
				if (notificationref[major][minor] == NULL) {
					notification = notify_notification_new(TEXT_TOPIC, notifystr, icon);
					notificationref[major][minor] = notification;
				} else {
					notification = notificationref[major][minor];
					notify_notification_update(notification, TEXT_TOPIC, notifystr, icon);
				}

			        notify_notification_set_timeout(notification, NOTIFICATION_TIMEOUT);
				notify_notification_set_category(notification, PROGNAME);
				notify_notification_set_urgency (notification, NOTIFY_URGENCY_NORMAL);
	
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
