LDLIBS ?= -lX11 -lXrender
CFLAGS ?= -g -Wall
PREFIX ?= /usr

SOURCES = xelflut.c args.c client.c network.c x11.c xfds.c
HEADERS = xelflut.h xfds.h

.PHONY: clean

all: xelflut

xelflut: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

run:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./xelflut -u

clean:
	$(RM) xelflut

install:
	install -m 0755 xelflut "$(DESTDIR)$(PREFIX)/bin"
