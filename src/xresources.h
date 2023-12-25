#pragma once

// using boost optional instead of std because it needs c++17 which is
// not widely available yet

#include <vector>
#include <boost/optional.hpp>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct XResources {
	~XResources();

	Display *dpy{NULL};
	boost::optional<Window> win;
	boost::optional<Atom> winDelMsg;
	XVisualInfo vi;

	XImage *mainImage{NULL};
	XImage *statusImage{NULL};
};
