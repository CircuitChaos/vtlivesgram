#include "xresources.h"

SXResources::SXResources():
	dpy(NULL),
	mainImage(NULL),
	statusImage(NULL)
{
}

SXResources::~SXResources()
{
	if (statusImage)
		XDestroyImage(statusImage);

	if (mainImage)
		XDestroyImage(mainImage);

	// should we also free winDelMsg (Atom) here?

	if (win)
		XDestroyWindow(dpy, win.get());

	if (dpy)
		XCloseDisplay(dpy);
}
