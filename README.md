# xelflut

A multiplayer X11 canvas.

Based on the idea of [pixelflut](https://cccgoe.de/wiki/Pixelflut), as seen at EasterHegg '14.

Written directly against XRender so it should probably be pretty fast.

Version numbers approximate &pi;.

## The name

It's funny on so many levels, some of which come only with an understanding of the german language
and more-or-less (probably less) great math and programming humour.

## Features

* IPv4 + IPv6 support
* Configurable frame rates and limits
* Few dependencies / lightweight
* Slim codebase (~900 LoC)

# Building & Setup

## Dependencies

* A C compiler
* (GNU) make
* libx11(-dev)
* libxrender(-dev)

## Build process

Run `make`.

## Setup

Configuration is done by passing arguments. Check the output of `xelflut -h` to see what is possible.

To install `xelflut` to the system (not in any way required), run `make install`.

# Usage

Just run the resulting binary.

Run `./xelflut -h` for some info on what you can do.

Within the window, press `q` to quit or `c` to clear the canvas.

By default, clients are limited to 50 pixels per frame (to change this, use the `-l` argument),
25 frames are rendered every second (configurable with `-f`). Clients exceeding the limits will
be slowed down by default. This behaviour can be changed to one of the enforcement policies
`disconnect` (Disconnect clients exceeding the limits) or `ignore` (Ignore additional pixels until
the next frame) with the `-e <enforcer>` option.
Clients are allowed only one connection per host (though this can be circumvented by reasonably
clever attackers). All limits and checks can be disabled (for example, for performance testing)
by passing the `-u` option.

# Protocol

Lines of ASCII text commands separated by `\n` via TCP on port `3141`, unless you
configure it differently (arguments `-b` and `-p`).


Send `SIZE` and the server responds with `SIZE WIDTH HEIGHT`

Send `PX X Y RRGGBB` or `PX X Y RRGGBBAA` to set a pixel to a hex color code.

There are some measures in place to limit the number of connections and pixels a client
may have and set. Some are even configurable.

# Bugs & Feedback

Please report bugs via the issue tracker. 

If you use this on some kind of humungous screen or at a cool event, send me a picture (cb@cbcdn.com)!
