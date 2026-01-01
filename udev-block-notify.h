/*
 * (C) 2011-2026 by Christian Hesse <mail@eworm.de>
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

/*** received_signal ***/
void received_signal(int signal);

/*** main ***/
int main (int argc, char ** argv);

#endif /* UDEV_BLOCK_NOTIFY_H */
