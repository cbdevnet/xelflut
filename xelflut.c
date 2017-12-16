#include "xelflut.h"

int usage(char* fn){
	printf("%s - Multiplayer X11 canvas\n", XELFLUT_VERSION);
	printf("Recognized parameters:\n");
	printf("-b <bindhost>\tInterface to bind to (Default ::)\n");
	printf("-p <port>\tPort to listen for connections on (Default 3141)\n");
	printf("-l <limit>\tPixels a client may set per frame (Default 50)\n");
	printf("-f <framerate>\tMaximum framerate (Default 25)\n");
	printf("-d <width>x<height>\tThe canvas size (Default 800x600)\n");
	printf("-u\t\tUnsafe mode - disregard client checks and limits\n");
	printf("-x\t\tOnly accept IPv4/6 clients as indicated by bindhost\n");
	printf("-w\t\tDo not fullscreen the window\n");
	printf("-s\t\tForce square pixels\n");

	return EXIT_FAILURE;
}

void signal_handler(int signum){
	abort_signaled = 1;
}

int main(int argc, char** argv){
	int listen_fd = -1, maxfd, error;
	struct timespec current_time;
	struct timeval select_timeout;
	int delta_nanos;
	fd_set readfds;

	//parse arguments
	if(args_parse(argc, argv)){
		return usage(argv[0]);
	}

	//handle SIGINT
	signal(SIGINT, signal_handler);

	//create sockets
	listen_fd = tcp_listener(config.bindhost, config.port);
	if(listen_fd < 0){
		fprintf(stderr, "Failed to create listening socket\n");
		return usage(argv[0]);
	}

	//create window
	x11_init();
	//flush events
	x11_handle();

	//wait for events
	while(!abort_signaled){
		//reset fd set
		FD_ZERO(&readfds);

		//add listen fd
		FD_SET(listen_fd, &readfds);
		maxfd = listen_fd;

		//add all clients
		clients_add(&readfds, &maxfd);

		//add all x fds
		xfds_select(&x11.xfds, &readfds, &maxfd);

		//set up timeout
		select_timeout.tv_sec = 0;
		select_timeout.tv_usec = (1e6 / config.frame_rate) / 2;

		//wait for events
		error = select(maxfd + 1, &readfds, NULL, NULL, &select_timeout);
		if(error < 0){
			perror("select");
			break;
		}

		if(error > 0){
			//handle events
			x11_handle();
			if(FD_ISSET(listen_fd, &readfds)){
				client_accept(listen_fd);
			}
		}
		clients_handle(&readfds);

		//if frame time elapsed, draw and reset the limits
		clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
		delta_nanos = current_time.tv_nsec - x11.last_render.tv_nsec;
		delta_nanos = (delta_nanos < 0) ? 1e9 + delta_nanos:delta_nanos;
		if(delta_nanos > 1e9 / config.frame_rate){
			if(current_time.tv_sec != x11.last_render.tv_sec){
				fprintf(stderr, "Drawing at %f fps\n", 1e9/delta_nanos);
			}
			x11_redraw();
			clients_relimit();
			x11.last_render = current_time;
		}
	}

	//cleanup
	x11_cleanup();
	clients_cleanup();
	close(listen_fd);
	return EXIT_SUCCESS;
}
