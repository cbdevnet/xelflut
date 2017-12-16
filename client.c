bool client_same(client* a, client* b){
	struct sockaddr_in6* a6 = (struct sockaddr_in6*)&(a->peer);
	struct sockaddr_in6* b6 = (struct sockaddr_in6*)&(b->peer);
	struct sockaddr_in* a4 = (struct sockaddr_in*)&(a->peer);
	struct sockaddr_in* b4 = (struct sockaddr_in*)&(b->peer);

	if(a->peer.ss_family == b->peer.ss_family){
		switch(a->peer.ss_family){
			case AF_INET:
				return memcmp(&a4->sin_addr, &b4->sin_addr, sizeof(a4->sin_addr)) == 0;
			case AF_INET6:
				return memcmp(&a6->sin6_addr, &b6->sin6_addr, sizeof(a6->sin6_addr)) == 0;
			default:
				//unknown protocol type, disconnect
				return true;
		}
	}
	return false;
}

void clients_add(fd_set* fds, int* maxfd){
	size_t u;
	for(u = 0; u < clients.length; u++){
		if(clients.entries[u].fd >= 0){
			FD_SET(clients.entries[u].fd, fds);
			*maxfd = (clients.entries[u].fd > *maxfd) ? clients.entries[u].fd:*maxfd;
		}
	}
}

void clients_relimit(){
	size_t u, c;
	for(u = 0; u < clients.length; u++){
		c += clients.entries[u].submits;
		clients.entries[u].submits = 0;
	}

	config.pixels += c;
}

int client_disconnect(client* disc){
	client empty_client = {
		.fd = -1
	};

	config.clients--;
	close(disc->fd);
	*disc = empty_client;

	fprintf(stderr, "Disconnected client in slot %zi\n", disc - clients.entries);
	return 0;
}

int client_accept(int listen_fd){
	size_t u;
	size_t position;
	client empty_client = {
		.fd = -1
	};

	for(position = 0; position < clients.length; position++){
		if(clients.entries[position].fd < 0){
			break;
		}
	}

	if(position == clients.length){
		clients.entries = realloc(clients.entries, (position + 1) * sizeof(client));
		if(!clients.entries){
			fprintf(stderr, "Failed to allocate memory");
			//harsh but well
			exit(EXIT_FAILURE);
		}
		clients.length = position + 1;
	}
	clients.entries[position] = empty_client;

	clients.entries[position].fd = accept(listen_fd,(struct sockaddr*) &clients.entries[position].peer, &clients.entries[position].peer_len);
	config.clients++;

	if(clients.entries[position].fd < 0){
		perror("accept");
		return -1;
	}

	fprintf(stderr, "Client in slot %zu\n", position);

	if(!config.unsafe){
		//check for multiple connections
		for(u = 0; u < clients.length; u++){
			if(u != position && clients.entries[u].fd >= 0 && client_same(clients.entries + position, clients.entries + u)){
				fprintf(stderr, "Disconnecting duplicate client\n");
				client_disconnect(clients.entries + position);
				return -1;
			}
		}
	}
	return 0;
}

int client_process(client* client, bool recv_data){
	ssize_t bytes;
	char send_buffer[DATA_BUFFER_LEN];
	unsigned pixel_x, pixel_y;
	char pixel_raw[10];
	char* offset = NULL;
	XRenderColor pixel_color = {
		.red = 0x0000,
		.green = 0xFF00,
		.blue = 0x00FF,
		.alpha = 0xFFFF
	};

	if(recv_data && (sizeof(client->data) - client->data_offset) > 0){
		//read data
		bytes = recv(client->fd, client->data + client->data_offset, sizeof(client->data) - client->data_offset, 0);

		//check for errors
		if(bytes < 0){
			perror("recv");
			return client_disconnect(client);
		}
		//check if closed
		else if(bytes == 0){
			return client_disconnect(client);
		}

		client->data_offset += bytes;
	}

	while(client->data_offset > 3 && memchr(client->data, '\n', client->data_offset)){
		if(!strncmp(client->data, "PX ", 3)){
			//check pixel limit
			if(config.limit_handling != none && !config.unsafe && client->submits >= config.frame_limit){
				if(config.limit_handling == ignore){
					goto line_handled;
				}
				break;
			}
			//draw pixel
			//cant use strtok here to separate arguments because the separator needs to be kept
			offset = client->data + 3;
			pixel_x = strtoul(offset, &offset, 0);

			if(*offset == ' '){
				pixel_y = strtoul(offset, &offset, 0);
				if(*offset == ' '){
					offset++;
					bytes = strchr(offset, '\n') - offset;
					if(bytes == 6 || bytes == 8){
						//parse color
						strncpy(pixel_raw, offset, bytes);
						pixel_color = args_color(pixel_raw);
						//draw pixel
						XRenderFillRectangle(x11.display, PictOpOver, x11.canvas_handle, &pixel_color, min(pixel_x, config.width), min(pixel_y, config.height), 1, 1);
					}
				}
			}

			client->submits++;
		}
		else if(!strncmp(client->data, "SIZE", 4)){
			//slight deviation from original protocol, terminate size response with \n
			bytes = snprintf(send_buffer, sizeof(send_buffer), "SIZE %d %d\n", config.width, config.height);
			//unsafe send, but hey
			send(client->fd, send_buffer, bytes, 0);
		}
line_handled:

		//remove sentence
		bytes = (char*)memchr(client->data, '\n', client->data_offset) + 1 - client->data;
		memmove(client->data, client->data + bytes, client->data_offset - bytes);
		client->data_offset -= bytes;
	}

	//check if client hit the limits
	if(sizeof(client->data) - client->data_offset < 2){
		if(config.limit_handling == disconnect || client->submits < config.frame_limit){
			fprintf(stderr, "Client %zu disconnected: Limit exceeded\n", client - clients.entries);
			return client_disconnect(client);
		}
	}
	return 0;
}

void clients_cleanup(){
	size_t u;
	for(u = 0; u < clients.length; u++){
		if(clients.entries[u].fd >= 0){
			close(clients.entries[u].fd);
			clients.entries[u].fd = -1;
		}
	}
}

int clients_handle(fd_set* fds){
	size_t u;
	int rv = 0;
	for(u = 0; u < clients.length; u++){
		if(clients.entries[u].fd >= 0 && (FD_ISSET(clients.entries[u].fd, fds)
					|| clients.entries[u].data_offset)){
			rv += client_process(clients.entries + u, FD_ISSET(clients.entries[u].fd, fds));
		}
	}
	return rv;
}
