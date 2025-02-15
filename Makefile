.POSIX:
.SUFFIXES:

include config.mk

# flags for compiling
MAOMAOCPPFLAGS = -I. -DIM -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L \
	-DVERSION=\"$(VERSION)\" $(XWAYLAND)
MAOMAODEVCFLAGS = -g -Wpedantic -Wall -Wextra -Wdeclaration-after-statement \
	-Wno-unused-parameter -Wshadow -Wunused-macros -Werror=strict-prototypes \
	-Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types \
	-Wfloat-conversion  -Wno-unused-function -Wno-declaration-after-statement \
	-Wno-sign-compare -Wno-float-conversion

# CFLAGS / LDFLAGS
PKGS      = wayland-server xkbcommon libinput $(XLIBS)
MAOMAOCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(WLR_INCS) $(MAOMAOCPPFLAGS) $(MAOMAODEVCFLAGS) $(CFLAGS)
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` $(WLR_LIBS) -lm $(LIBS)

all: maomao
maomao: maomao.o util.o dwl-ipc-unstable-v2-protocol.o
	$(CC) maomao.o util.o dwl-ipc-unstable-v2-protocol.o $(MAOMAOCFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
maomao.o: maomao.c IM.h client.h config.h config.mk cursor-shape-v1-protocol.h \
	pointer-constraints-unstable-v1-protocol.h wlr-layer-shell-unstable-v1-protocol.h \
	xdg-shell-protocol.h \
	wlr-foreign-toplevel-management-unstable-v1-protocol.h \
	dwl-ipc-unstable-v2-protocol.h
util.o: util.c util.h
dwl-ipc-unstable-v2-protocol.o: dwl-ipc-unstable-v2-protocol.c dwl-ipc-unstable-v2-protocol.h

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@
pointer-constraints-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-foreign-toplevel-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/wlr-foreign-toplevel-management-unstable-v1.xml $@
dwl-ipc-unstable-v2-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/dwl-ipc-unstable-v2.xml $@
xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

dwl-ipc-unstable-v2-protocol.c:
	$(WAYLAND_SCANNER) private-code \
		protocols/dwl-ipc-unstable-v2.xml $@

config.h:
	cp config.def.h $@
clean:
	rm -f maomao *.o *-protocol.h *-protocol.c

dist: clean
	mkdir -p maomao-$(VERSION)
	cp -R LICENSE* Makefile CHANGELOG.md README.md client.h config.def.h \
		config.mk protocols maomao.c util.c util.h maomao.desktop \
		maomao-$(VERSION)
	tar -caf maomao-$(VERSION).tar.gz maomao-$(VERSION)
	rm -rf maomao-$(VERSION)

install: maomao
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/maomao
	cp -f maomao $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/maomao
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(DATADIR)/wayland-sessions
	cp -f maomao.desktop $(DESTDIR)$(DATADIR)/wayland-sessions/maomao.desktop
	chmod 644 $(DESTDIR)$(DATADIR)/wayland-sessions/maomao.desktop
	rm -f maomao *.o *-protocol.h *-protocol.c
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/maomao \
		$(DESTDIR)$(DATADIR)/wayland-sessions/maomao.desktop

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(MAOMAOCFLAGS) -o $@ -c $<
