# udev-block-notify - Notify about udev block events

CC	:= gcc
CFLAGS	+= $(shell pkg-config --cflags --libs libudev) \
	   $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: udev-block-notify.c
	$(CC) $(CFLAGS) -o udev-block-notify udev-block-notify.c \
		-DVERSION="\"$(VERSION)\""

clean:
	/bin/rm -f *.o *~ udev-block-notify
