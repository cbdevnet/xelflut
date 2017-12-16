#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>

#define XELFLUT_VERSION "Xelflut 3.1"
#define XELFLUT_CLASS "xelflut"
#define DATA_BUFFER_LEN 8192

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include "xfds.h"
#define min(a,b) (a < b ? a:b)

volatile sig_atomic_t abort_signaled = 0;

typedef struct /*_XELFLUT_CLIENT*/ {
	int fd;
	struct sockaddr_storage peer;
	socklen_t peer_len;
	unsigned submits;
	size_t data_offset;
	char data[DATA_BUFFER_LEN];
} client;

struct /*_XELFLUT_CONF*/ {
	char* port;
	char* bindhost;
	bool exclusive;

	enum {
		limit = 0,
		ignore,
		disconnect,
		none
	} limit_handling;
	unsigned width;
	unsigned height;
	bool windowed;
	bool square_pixels;
	bool unsafe;

	unsigned frame_limit;
	unsigned frame_rate;
	unsigned pixels;
	unsigned clients;

	char* window_name;
} config = {
	.port = "3141",
	.bindhost = "::",
	.exclusive = false,

	.limit_handling = limit,
	.width = 800,
	.height = 600,
	.windowed = false,
	.square_pixels = false,
	.unsafe = false,

	.frame_limit = 50,
	.frame_rate = 25,
	.pixels = 0,
	.clients = 0,

	.window_name = XELFLUT_VERSION
};

struct /*_XELFLUT_X11*/ {
	Display* display;
	int screen;
	Window main;
	unsigned width;
	unsigned height;
	Colormap colormap;
	XVisualInfo visual_info;
	Atom wm_delete;
	X_FDS xfds;
	struct timespec last_render;

	XRenderColor background;
	Pixmap canvas;
	Picture canvas_handle;
	Picture window_handle;
} x11 = {
	.display = NULL,
	.background = {
		.alpha = -1
	}
};

struct /*_XELFLUT_CLIENTS*/ {
	size_t length;
	client* entries;
} clients = {
	.length = 0,
	.entries = NULL
};

#include "network.c"
#include "xfds.c"
#include "args.c"
#include "client.c"
#include "x11.c"
