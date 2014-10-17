# udev-block-notify - Notify about udev block events

CC	:= gcc -std=c11
MD	:= markdown
INSTALL	:= install
RM	:= rm
CFLAGS	+= -O2 -Wall -Werror
CFLAGS	+= $(shell pkg-config --cflags --libs libudev)
CFLAGS	+= $(shell pkg-config --cflags --libs libnotify)
# this is just a fallback in case you do not use git but downloaded
# a release tarball...
VERSION := 0.7.4

all: udev-block-notify README.html

udev-block-notify: udev-block-notify.c version.h
	$(CC) $(CFLAGS) -o udev-block-notify udev-block-notify.c

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	echo "#ifndef VERSION" > $@
	echo "#define VERSION \"$(shell git describe --tags --long 2>/dev/null || echo ${VERSION})\"" >> $@
	echo "#endif" >> $@

README.html: README.md
	$(MD) README.md > README.html

install: install-bin install-doc

install-bin: udev-block-notify
	$(INSTALL) -D -m0755 udev-block-notify $(DESTDIR)/usr/bin/udev-block-notify
	$(INSTALL) -D -m0644 udev-block-notify.desktop $(DESTDIR)/etc/xdg/autostart/udev-block-notify.desktop

install-doc: README.html
	$(INSTALL) -D -m0644 README.md $(DESTDIR)/usr/share/doc/udev-block-notify/README.md
	$(INSTALL) -D -m0644 README.html $(DESTDIR)/usr/share/doc/udev-block-notify/README.html
	$(INSTALL) -D -m0644 screenshot.png $(DESTDIR)/usr/share/doc/udev-block-notify/screenshot.png

clean:
	$(RM) -f *.o *~ README.html udev-block-notify version.h

release:
	git archive --format=tar.xz --prefix=udev-block-notify-$(VERSION)/ $(VERSION) > udev-block-notify-$(VERSION).tar.xz
	gpg -ab udev-block-notify-$(VERSION).tar.xz
