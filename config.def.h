/*
 * (C) 2011-2020 by Christian Hesse <mail@eworm.de>
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

#ifndef CONFIG_H
#define CONFIG_H

/* how long to show notifications */
#define NOTIFICATION_TIMEOUT	10000

/* this icons shown */
#define ICON_DEVICE_MAPPER		"media-playlist-shuffle"
#define ICON_DRIVE_HARDDISK		"drive-harddisk"
#define ICON_DRIVE_HARDDISK_IEEE1394	"drive-harddisk-ieee1394"
#define ICON_DRIVE_HARDDISK_USB		"drive-harddisk-usb"
#define ICON_DRIVE_OPTICAL		"drive-optical"
#define ICON_DRIVE_MULTIDISK		"drive-multidisk"
#define ICON_LOOP			"media-playlist-repeat"
#define ICON_MEDIA_FLASH		"media-flash"
#define ICON_MEDIA_FLOPPY		"media-floppy"
#define ICON_MEDIA_OPTICAL_CD_AUDIO	"media-optical-cd-audio"
#define ICON_MEDIA_REMOVABLE		"media-removable"
#define ICON_MEDIA_ZIP			"media-zip"
#define ICON_MULTIMEDIA_PLAYER		"multimedia-player"
#define ICON_NETWORK_SERVER		"network-server"
#define ICON_UNKNOWN			"dialog-question"

/* the text displayed */
#define TEXT_TOPIC	"Udev Block Notification"
#define TEXT_ADD	"Device <b>%s</b> (%i:%i) <b>appeared</b>."
#define TEXT_REMOVE	"Device <b>%s</b> (%i:%i) <b>disappeared</b>."
#define TEXT_MOVE	"Device <b>%s</b> (%i:%i) was <b>renamed</b>."
#define TEXT_CHANGE	"Device <b>%s</b> (%i:%i) media <b>changed</b>."
#define TEXT_DEFAULT	"Anything happend to <b>%s</b> (%i:%i)... Don't know."
#define TEXT_TAG	"\n%s: <i>%s</i>"

#endif /* CONFIG_H */
