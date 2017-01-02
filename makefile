LDLIBS ?= -lX11 -lXrender
CFLAGS ?= -g -Wall

.PHONY: clean

all: xelflut

run:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./xelflut

clean:
	$(RM) xelflut
