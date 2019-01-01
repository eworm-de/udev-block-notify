/*
 * (C) 2011-2019 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#ifndef UDEV_BLOCK_NOTIFY_H
#define UDEV_BLOCK_NOTIFY_H

#define _GNU_SOURCE

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

/* systemd headers */
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <libnotify/notify.h>
#include <libudev.h>

#define PROGNAME	"udev-block-notify"

struct notifications {
	dev_t devnum;
	NotifyNotification *notification;
	struct notifications *next;
};

/*** get_notification ***/
NotifyNotification * get_notification(struct notifications *notifications,
		dev_t devnum);

/*** newstr ***/
char * newstr(const char *text, const char *device, unsigned short int major,
		unsigned short int minor);

/*** appendstr ***/
char * appendstr(const char *text, char *notifystr, const char *property,
		const char *value);

/*** main ***/
int main (int argc, char ** argv);

#endif /* UDEV_BLOCK_NOTIFY_H */
