XRenderColor args_color(char* raw){
	XRenderColor color = {
		.alpha = -1
	};

	uint32_t mask = 0xFF;
	uint32_t bytes = strtoul(raw, NULL, 16);

	if(strlen(raw) == 8){
		color.alpha = (bytes & mask) << 8 | (bytes & mask);
		bytes >>= 8;
	}

	color.blue = (bytes & mask) << 8 | (bytes & mask);
	bytes >>= 8;

	color.green = (bytes & mask) << 8 | (bytes & mask);
	bytes >>= 8;

	color.red = (bytes & mask) << 8 | (bytes & mask);
	bytes >>= 8;

	//fprintf(stderr, "Read color r:%X g:%X b:%X a:%X\n", color.red, color.green, color.blue, color.alpha);
	return color;
}

int args_parse(int argc, char** argv){
	unsigned u;
	char* offset;
	for(u = 1; u < argc; u++){
		if(argv[u][0] == '-'){
			switch(argv[u][1]){
				case 'b':
					config.bindhost = argv[u + 1];
					u++;
					break;
				case 'p':
					config.port = argv[u + 1];
					u++;
					break;
				case 'l':
					config.frame_limit = strtoul(argv[u + 1], NULL, 0);
					u++;
					break;
				case 'f':
					config.frame_rate = strtoul(argv[u + 1], NULL, 0);
					u++;
					break;
				case 'd':
					if(argv[u + 1]){
						config.width = strtoul(argv[u + 1], &offset, 0);
						if(*offset != 'x'){
							fprintf(stderr, "Invalid dimension specification\n");
							return -1;
						}
						config.height = strtoul(offset + 1, NULL, 0);
					}
					u++;
					break;
				case 'e':
					if(!strcmp(argv[u + 1], "ignore")){
						config.limit_handling = ignore;
					}
					else if(!strcmp(argv[u + 1], "disconnect")){
						config.limit_handling = disconnect;
					}
					else if(!strcmp(argv[u + 1], "none")){
						config.limit_handling = none;
					}
					else{
						config.limit_handling = limit;
					}
					u++;
					break;
				case 'u':
					config.unsafe = true;
					break;
				case 'x':
					config.exclusive = true;
					break;
				case 'w':
					config.windowed = true;
					break;
				case 's':
					config.scale_uniform = true;
					break;
				case 'c':
					config.centered = true;
					break;
				default:
					fprintf(stderr, "Unknown option: %s\n", argv[u]);
					return -1;
			}
		}
		else{
			fprintf(stderr, "Unknown argument: %s\n", argv[u]);
			return -1;
		}
	}

	if(u - argc > 0){
		fprintf(stderr, "Option expects a parameter: %s\n", argv[argc - 1]);
		return -1;
	}
	return 0;
}
