# xelflut

A multiplayer X11 canvas.

Based on the idea of [pixelflut](https://cccgoe.de/wiki/Pixelflut), as seen at EasterHegg '14.

Written directly against XRender so it should probably be pretty fast.

Version numbers approximate &Pi;.

## The name

It's funny on so many levels, some of which come only with an understanding of the german language
and more-or-less (probably less) great math and programming humour.

# Building & Setup

## Dependencies

* A C compiler
* make
* libx11(-dev)
* libxrender(-dev)

## Build process

Run `make`.

## Setup

None as of yet. Just run the resulting binary.

# Usage

Run `./xelflut -h` for some info on what you can do.

# Protocol

Lines of ASCII text commands separated by `\n`.


Send `SIZE` and the server responds with `SIZE WIDTH HEIGHT`

Send `PX X Y RRGGBB` or `PX X Y RRGGBBAA` to set a pixel to a hex color code.

There are some measures in place to limit the number of connections and pixels a client
may have and set. Some are even configurable.

# Bugs & Feedback

Please report bugs via the issue tracker. 

If you use this on some kind of humungous screen or at a cool event, send me a picture (cb@cbcdn.com)!
