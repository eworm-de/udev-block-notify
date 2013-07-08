# udev-block-notify - Notify about udev block events

CC	:= gcc
INSTALL	:= install
RM	:= rm
CFLAGS	+= -O2 -Wall -Werror
CFLAGS	+= $(shell pkg-config --cflags --libs libudev) \
	   $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: udev-block-notify.c
	$(CC) $(CFLAGS) -o udev-block-notify udev-block-notify.c \
		-DVERSION="\"$(VERSION)\""

install:
	$(INSTALL) -D -m0755 udev-block-notify $(DESTDIR)/usr/bin/udev-block-notify
	$(INSTALL) -D -m0644 udev-block-notify.desktop $(DESTDIR)/etc/xdg/autostart/udev-block-notify.desktop

clean:
	$(RM) -f *.o *~ udev-block-notify
