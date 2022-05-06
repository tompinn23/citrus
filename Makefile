WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

LIBS=\
	 $(shell pkg-config --cflags --libs wlroots) \
	 $(shell pkg-config --cflags --libs wayland-server) \
	 $(shell pkg-config --cflags --libs xkbcommon)

.PHONY: all
all: citrus

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@


citrus: citrus.c xdg-shell-protocol.h
	$(CC) $(CFLAGS) \
		-g -Werror -I. -I/usr/local/include \
		-DWLR_USE_UNSTABLE=1 \
		-L/usr/local/lib \
		-o $@ $< \
		$(LIBS)

.PHONY: clean
clean:
	rm -f citrus xdg-shell-protocol.h

