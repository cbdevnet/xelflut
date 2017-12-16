int x11_init(){
	int status;
	Window root;
	XSetWindowAttributes window_attributes;
	Atom wm_state_fullscreen;
	XTextProperty window_name;
	pid_t pid = getpid();

	XSizeHints* size_hints = NULL;
	XWMHints* wm_hints = NULL;
	XClassHint* class_hints = NULL;

	//x data initialization
	x11.display = XOpenDisplay(NULL);

	if(!(x11.display)){
		fprintf(stderr, "Failed to open display\n");
		return -1;
	}

	if(!XRenderQueryExtension(x11.display, &status, &status)){
		fprintf(stderr, "Missing XRender support on the display\n");
		return -1;
	}

	x11.screen = DefaultScreen(x11.display);
	root = RootWindow(x11.display, x11.screen);

	if(!XMatchVisualInfo(x11.display, x11.screen, 32, TrueColor, &(x11.visual_info))) {
		fprintf(stderr, "Display does not support RGBA TrueColor visual\n");
		return -1;
	}

	x11.colormap = XCreateColormap(x11.display, root, x11.visual_info.visual, AllocNone);

	//set up window params
	window_attributes.background_pixel = XBlackPixel(x11.display, x11.screen);
	window_attributes.border_pixel = XBlackPixel(x11.display, x11.screen);
	window_attributes.colormap = x11.colormap;
	window_attributes.cursor = None;
	window_attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
	x11.width = DisplayWidth(x11.display, x11.screen);
	x11.height = DisplayHeight(x11.display, x11.screen);

	if(config.windowed){
		x11.width = config.width;
		x11.height = config.height;
	}

	//create window
	x11.main = XCreateWindow(x11.display,
				root,
				0,
				0,
				x11.width,
				x11.height,
				0,
				32,
				InputOutput,
				x11.visual_info.visual,
				CWBackPixel | CWCursor | CWEventMask | CWBorderPixel | CWColormap,
				&window_attributes);

	//set window properties
	if(XStringListToTextProperty(&(config.window_name), 1, &window_name) == 0){
		fprintf(stderr, "Failed to create string list, aborting\n");
		return -1;
	}

	//allocate some structures
	size_hints = XAllocSizeHints();
	wm_hints = XAllocWMHints();
	class_hints = XAllocClassHint();

	if(!size_hints || !wm_hints || !class_hints){
		fprintf(stderr, "Failed to allocate X data structures\n");
		return -1;
	}

	wm_hints->flags = 0;
	class_hints->res_name = XELFLUT_VERSION;
	class_hints->res_class = XELFLUT_CLASS;

	XSetWMProperties(x11.display, x11.main, &window_name, NULL, NULL, 0, NULL, wm_hints, class_hints);

	XFree(window_name.value);
	XFree(size_hints);
	XFree(wm_hints);
	XFree(class_hints);

	//set fullscreen mode
	if(!config.windowed){
		wm_state_fullscreen = XInternAtom(x11.display, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(x11.display, x11.main, XInternAtom(x11.display, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (unsigned char*) &wm_state_fullscreen, 1);
	}

	XChangeProperty(x11.display, x11.main, XInternAtom(x11.display, "_NET_WM_PID", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&pid, 1);

	//register for WM_DELETE_WINDOW messages
	x11.wm_delete = XInternAtom(x11.display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(x11.display, x11.main, &(x11.wm_delete), 1);

	//map window
	XMapRaised(x11.display, x11.main);

	//get x socket fds
	if(!xfd_add(&(x11.xfds), XConnectionNumber(x11.display))){
		fprintf(stderr, "Failed to allocate xfd memory\n");
		return -1;
	}
	if(XAddConnectionWatch(x11.display, xconn_watch, (void*)(&(x11.xfds))) == 0){
		fprintf(stderr, "Failed to register connection watch procedure\n");
		return -1;
	}

	fprintf(stderr, "Creating canvas of dimensions %dx%d\n", config.width, config.height);
	x11.canvas = XCreatePixmap(x11.display, x11.main, config.width, config.height, 32);
	x11.canvas_handle = XRenderCreatePicture(x11.display, x11.canvas, XRenderFindStandardFormat(x11.display, PictStandardARGB32), 0, 0);
	x11.window_handle = XRenderCreatePicture(x11.display, x11.main, XRenderFindStandardFormat(x11.display, PictStandardARGB32), 0, 0);

	//clear pixmap
	XRenderFillRectangle(x11.display, PictOpOver, x11.canvas_handle, &x11.background, 0, 0, config.width, config.height);

	if(!x11.canvas){
		fprintf(stderr, "Failed to create canvas pixmap\n");
		return -1;
	}

	return 0;
}

void x11_redraw(){
	XEvent event = {
		.type = Expose
	};
	XSendEvent(x11.display, x11.main, False, 0, &event);
	XFlush(x11.display);
}

void x11_handle(){
	XEvent event;
	char pressed_key;
	bool reconfigured = false, exposed = false;
	XTransform transform = {{
		{XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0)},
		{XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}
	}};
	//handle events
	while(XPending(x11.display)){
		XNextEvent(x11.display, &event);
		//handle events
		switch(event.type){
			case ConfigureNotify:
				x11.width = event.xconfigure.width;
				x11.height = event.xconfigure.height;
				reconfigured = true;
				break;

			case Expose:
				exposed = true;
				break;

			case KeyPress:
				//translate key event into a character, respecting keyboard layout
				if(XLookupString(&event.xkey, &pressed_key, 1, NULL, NULL) != 1){
					//disregard combined characters / bound strings
					break;
				}
				switch(pressed_key){
					case 'q':
						abort_signaled = 1;
						break;
					case 'r':
						fprintf(stderr, "Redrawing on request\n");
						x11_redraw();
						break;
					case 'c':
						fprintf(stderr, "Clearing image data\n");
						XRenderFillRectangle(x11.display, PictOpOver, x11.canvas_handle, &x11.background, 0, 0, config.width, config.height);
						break;
					default:
						fprintf(stderr, "KeyPress %d (%c)\n", event.xkey.keycode, pressed_key);
						break;
				}
				break;

			case ClientMessage:
				if(event.xclient.data.l[0] == x11.wm_delete){
					fprintf(stderr, "Closing down window\n");
					abort_signaled = 1;
				}
				else{
					fprintf(stderr, "Client message\n");
				}
				break;

			default:
				fprintf(stderr, "Unhandled X event\n");
				break;
		}
	}

	XFlush(x11.display);

	if(reconfigured){

		//update transform scale
		transform.matrix[2][2] = XDoubleToFixed(min(x11.width / (double) config.width, x11.height / (double) config.height));
		fprintf(stderr, "Window configured to %dx%d, image scale %f\n", x11.width, x11.height, min(x11.width / (double) config.width, x11.height / (double) config.height));
		XRenderSetPictureTransform(x11.display, x11.canvas_handle, &transform);
	}

	if(exposed){
		//clear window //FIXME is this even needed (ie, is the image transparent?)
		//XClearWindow(x11.display, x11.main);
		XRenderFillRectangle(x11.display, PictOpOver, x11.window_handle, &x11.background, 0, 0, x11.width, x11.height);

		//TODO update transformation matrix

		//composite pixmap onto window
		XRenderComposite(x11.display, PictOpOver, x11.canvas_handle, None, x11.window_handle, 0, 0, 0, 0, 0, 0, x11.width, x11.height);
	}
}

void x11_cleanup(){
	if(!x11.display){
		return;
	}

	XRenderFreePicture(x11.display, x11.canvas_handle);
	XRenderFreePicture(x11.display, x11.window_handle);

	if(x11.main){
		XDestroyWindow(x11.display, x11.main);
	}

	if(x11.colormap){
		XFreeColormap(x11.display, x11.colormap);
	}

	XFreePixmap(x11.display, x11.canvas);
	XCloseDisplay(x11.display);
	xfd_free(&(x11.xfds));
}
