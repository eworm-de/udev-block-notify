/*
 * (C) 2011-2015 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#ifndef CONFIG_H
#define CONFIG_H

#define NOTIFICATION_TIMEOUT	10000

#define ICON_DEVICE_MAPPER		"media-playlist-shuffle"
#define ICON_DRIVE_HARDDISK		"drive-harddisk"
#define ICON_DRIVE_HARDDISK_IEEE1394	"drive-harddisk-ieee1394"
#define ICON_DRIVE_HARDDISK_USB		"drive-harddisk-usb"
#define ICON_DRIVE_OPTICAL		"drive-optical"
#define ICON_DRIVE_MULTIDISK		"drive-multidisk"
#define ICON_LOOP			"media-playlist-repeat"
#define ICON_MEDIA_FLASH		"media-flash"
#define ICON_MEDIA_FLOPPY		"media-floppy"
#define ICON_MEDIA_REMOVABLE		"media-removable"
#define ICON_MEDIA_ZIP			"media-zip"
#define ICON_MULTIMEDIA_PLAYER		"multimedia-player"
#define ICON_NETWORK_SERVER		"network-server"
#define ICON_UNKNOWN			"dialog-question"

#define TEXT_TOPIC	"Udev Block Notification"
#define TEXT_ADD	"Device <b>%s</b> (%i:%i) <b>appeared</b>."
#define TEXT_REMOVE	"Device <b>%s</b> (%i:%i) <b>disappeared</b>."
#define TEXT_MOVE	"Device <b>%s</b> (%i:%i) was <b>renamed</b>."
#define TEXT_CHANGE	"Device <b>%s</b> (%i:%i) media <b>changed</b>."
#define TEXT_DEFAULT	"Anything happend to <b>%s</b> (%i:%i)... Don't know."
#define TEXT_TAG	"\n%s: <i>%s</i>"

#endif /* CONFIG_H */
