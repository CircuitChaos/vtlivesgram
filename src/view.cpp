#include <X11/Xutil.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cmath>
#include "view.h"
#include "throw.h"
#include "font.h"
#include "soxpal.h"

static const unsigned INITIAL_WIDTH	= 800;
static const unsigned INITIAL_HEIGHT	= 600;
static const unsigned STATUS_HEIGHT	= font::CHAR_HEIGHT;	// do not change without changing drawStatusChar()
static const unsigned SIGNAL_DBFS_MAX	= 12000;
static const double MIN_SPEED		= 0.01;
static const double SPEED_SCALE_FACTOR	= 1.1;
static const unsigned SHIFT_STEP_DIVISOR	= 20;

static const char WINDOW_NAME[] = "vtlivesgram";

#if 0
static int errorHandler(Display *dpy, XErrorEvent *evt)
{
	char buf[512];
	XGetErrorText(dpy, evt->error_code, buf, sizeof(buf) - 1);
	xthrow("X error: %s", buf);
	/* NOTREACHED */
	return 0;
}

static int ioErrorHandler(Display *dpy)
{
	xthrow("X I/O error");
	/* NOTREACHED */
	return 0;
}
#endif

CView::CView():
	m_fd(-1),
	m_width(0),
	m_height(0),
	m_speed(1.0),
	m_zoom(1),
	m_shift(0),
	m_mousex(-1),
	m_lastRate(0),
	m_lastTs(0),
	m_lastFftWindowName("?"),
	m_signalHeight(0),
	m_waterfallHeight(0),
	m_freeze(false)
{
	m_res.dpy = XOpenDisplay(NULL);
	xassert(m_res.dpy, "Cannot open display");
	m_fd = ConnectionNumber(m_res.dpy);

	const int screen(DefaultScreen(m_res.dpy));

	xassert(XMatchVisualInfo(m_res.dpy, screen, 24, TrueColor, &m_res.vi) != 0, "This program needs at least 24-bit display");

	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(m_res.dpy, DefaultRootWindow(m_res.dpy), m_res.vi.visual, AllocNone);
	attr.background_pixel = 0;
	attr.border_pixel = 0;

	m_res.win = XCreateWindow(
		m_res.dpy,				// display
		DefaultRootWindow(m_res.dpy),		// parent
		0, 0,					// x and y position
		INITIAL_WIDTH, INITIAL_HEIGHT,		// width and height
		0,					// border width
		m_res.vi.depth,				// depth
		InputOutput,				// class
		m_res.vi.visual,			// visual
		CWColormap | CWBackPixel | CWBorderPixel,	// valuemask
		&attr);					// attributes

	m_res.winDelMsg = XInternAtom(m_res.dpy, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(m_res.dpy, m_res.win.get(), &m_res.winDelMsg.get(), 1);
	XStoreName(m_res.dpy, m_res.win.get(), WINDOW_NAME);
	XSelectInput(m_res.dpy, m_res.win.get(),
		PointerMotionMask |	// for updating frequency on status bar
		ButtonPressMask |	// for wheel (button 4 and 5)
		KeyPressMask |		// for keyboard events
		ExposureMask |		// for redrawing image on window
		StructureNotifyMask);	// for resizing
	XMapWindow(m_res.dpy, m_res.win.get());

	XWindowAttributes a;
	XGetWindowAttributes(m_res.dpy, m_res.win.get(), &a);
	m_width = a.width;
	m_height = a.height;
	resetImages();
}

CView::~CView()
{
}

int CView::getFd() const
{
	return m_fd;
}

unsigned CView::getWidth() const
{
	return m_width * m_zoom;
}

double CView::getSpeed() const
{
	return m_speed;
}

uint32_t CView::evt()
{
	uint32_t rs(EVT_NONE);

	while (XPending(m_res.dpy) > 0)
	{
		XEvent e;
		XNextEvent(m_res.dpy, &e);

		switch (e.type)
		{
			// PointerMotionMask
			case MotionNotify:
				xassert(e.xmotion.type == MotionNotify, "Invalid event type: %d", e.xmotion.type);
				updateMouse((unsigned) e.xmotion.x, (unsigned) e.xmotion.y);
				break;

			// ButtonPressMask
			case ButtonPress:
				xassert(e.xbutton.type == ButtonPress, "Invalid event type: %d", e.xbutton.type);
				switch (e.xbutton.button)
				{
					case Button1:
						rs |= EVT_NEXT_FFT_WINDOW;
						break;

					case Button3:
						updateFreeze();
						break;

					case Button4:
						rs |= updateSpeed(DIR_UP) ? EVT_CONFIG_CHANGED : 0;
						break;

					case Button5:
						rs |= updateSpeed(DIR_DN) ? EVT_CONFIG_CHANGED : 0;
						break;

					default:
						break;
				}
				break;

			// KeyPressMask
			case KeyPress:
			{
				xassert(e.xkey.type == KeyPress, "Invalid event type: %d", e.xkey.type);
				KeySym ks(0);
				XLookupString((XKeyEvent *) &e, NULL, 0, &ks, NULL);
				switch (ks)
				{
					case 'q':
						rs |= EVT_TERMINATE;
						break;

					case '+':
					case '=':
					case 0xffab:	// num+
						rs |= updateSpeed(DIR_UP) ? EVT_CONFIG_CHANGED : 0;
						break;

					case '-':
					case 0xffad:	// num-
						rs |= updateSpeed(DIR_DN) ? EVT_CONFIG_CHANGED : 0;
						break;

					case 0xff08:	// bksp
						rs |= updateSpeed(DIR_RESET) ? EVT_CONFIG_CHANGED : 0;
						break;

					case 0x20:	// space
						updateFreeze();
						break;

					case 'w':
						rs |= EVT_NEXT_FFT_WINDOW;
						break;

					case 0xff52:	// up
						rs |= updateZoom(true) ? EVT_CONFIG_CHANGED : 0;
						break;

					case 0xff54:	// down
						rs |= updateZoom(false) ? EVT_CONFIG_CHANGED : 0;
						break;

					case 0xff53:	// right
						updateShift(true);
						break;

					case 0xff51:	// left
						updateShift(false);
						break;

					case 'r':
						rs |= resetZoomAndShift() ? EVT_CONFIG_CHANGED : 0;
						break;

					default:
						break;
				}
				break;
			}

			// ExposureMask
			case Expose:
				redrawAll();
				break;

			// StructureNotifyMask
			case ConfigureNotify:
				xassert(e.xconfigure.type == ConfigureNotify, "Invalid event type: %d", e.xconfigure.type);
				if ((int) m_width == e.xconfigure.width && (int) m_height == e.xconfigure.height)
					break;

				if ((int) m_width != e.xconfigure.width)
					rs |= EVT_CONFIG_CHANGED;

				m_width = e.xconfigure.width;
				m_height = e.xconfigure.height;
				resetImages();
				if (m_speed > getMaxSpeed())
				{
					// normalize speed
					if (updateSpeed(DIR_UP))
						rs |= EVT_CONFIG_CHANGED;
				}

				if (m_shift > getMaxShift())
				{
					m_shift = getMaxShift();
					rs |= EVT_CONFIG_CHANGED;
				}

				redrawAll();
				break;

			// StructureNotifyMask
			case MapNotify:
				redrawAll();
				break;

			// from window manager
			case ClientMessage:
				xassert(e.xclient.type == ClientMessage, "Invalid event type: %d", e.xclient.type);
				if ((Atom) e.xclient.data.l[0] == m_res.winDelMsg.get())
					rs |= EVT_TERMINATE;
				break;

			default:
				break;
		}
	}

	return rs;
}

void CView::update(const std::vector<uint16_t> &data, uint32_t rate, uint64_t ts, const std::string &fftWindowName)
{
	if (m_freeze)
		return;

	if (data.size() != m_width * m_zoom)
		return;

	if (m_zoom == 1)
		m_lastData = data;
	else
	{
		xassert((m_shift + m_width) <= (m_width * m_zoom), "%u %u %u", m_shift, m_width, m_zoom);
		m_lastData.clear();
		m_lastData.reserve(m_width);
		std::copy(data.begin() + m_shift, data.begin() + m_shift + m_width, std::back_inserter(m_lastData));
	}

	m_lastRate = rate;
	m_lastTs = ts;
	m_lastFftWindowName = fftWindowName;

	drawSignal();
	updateWaterfall();
	updateStatus();
	redrawAll();
}

void CView::redrawAll()
{
	if (m_res.mainImage)
		XPutImage(
			m_res.dpy,			// display
			m_res.win.get(),		// drawable
			DefaultGC(m_res.dpy, DefaultScreen(m_res.dpy)),	// GC
			m_res.mainImage,		// image
			0, 0,				// src_x, src_y
			0, 0,				// dest_x, dest_y
			m_width, m_height - STATUS_HEIGHT);	// width, height

	redrawStatus();
}

void CView::redrawStatus()
{
	if (m_res.statusImage)
		XPutImage(
			m_res.dpy,			// display
			m_res.win.get(),		// drawable
			DefaultGC(m_res.dpy, DefaultScreen(m_res.dpy)),	// GC
			m_res.statusImage,		// image
			0, 0,				// src_x, src_y
			0, m_height - STATUS_HEIGHT, 	// dest_x, dest_y
			m_width, STATUS_HEIGHT);	// width, height
}

void CView::resetImages()
{
	if (m_res.statusImage)
		XDestroyImage(m_res.statusImage);

	if (m_res.mainImage)
		XDestroyImage(m_res.mainImage);

	m_res.statusImage = NULL;
	m_res.mainImage = NULL;

	xassert(m_width > 0 && m_height > 0, "Refusing to create image with zero size (%u x %u)", m_width, m_height);
	if (m_height <= STATUS_HEIGHT)
	{
		// window too small
		return;
	}

	// we need to allocate this memory this way, as XDestroyImage
	// frees it for us

	const unsigned mainHeight(m_height - STATUS_HEIGHT);
	char * const mainPixels((char *) malloc(m_width * mainHeight * 4));
	xassert(mainPixels, "Could not allocate memory for image");
	char * const statusPixels((char *) malloc(m_width * STATUS_HEIGHT * 4));
	xassert(statusPixels, "Could not allocate memory for status bar");

	m_res.mainImage = XCreateImage(
		m_res.dpy,			// display
		m_res.vi.visual,		// visual
		m_res.vi.depth,			// depth
		ZPixmap,			// format
		0,				// offset
		mainPixels,			// data
		m_width,			// width
		mainHeight,			// height
		32,	// bitmap_pad -- not sure
		0);	// bytes_per_line

	m_res.statusImage = XCreateImage(
		m_res.dpy,			// display
		m_res.vi.visual,		// visual
		m_res.vi.depth,			// depth
		ZPixmap,			// format
		0,				// offset
		statusPixels,			// data
		m_width,			// width
		STATUS_HEIGHT,			// height
		32,	// bitmap_pad -- not sure
		0);	// bytes_per_line

	m_signalHeight = mainHeight / 4;
	m_waterfallHeight = mainHeight - m_signalHeight;

	clearSignal();
	memset(m_res.mainImage->data + (m_width * m_signalHeight * 4), 0x00, m_width * m_waterfallHeight * 4);

	updateStatus();
}

void CView::updateMouse(unsigned x, unsigned /* y */)
{
	m_mousex = x;
	updateStatus();
	redrawStatus();
}

bool CView::updateSpeed(EDir dir)
{
	if (dir == DIR_UP)
	{
		double newSpeed(m_speed * SPEED_SCALE_FACTOR);
		if (newSpeed >= getMaxSpeed())
			newSpeed = getMaxSpeed();

		if (fabs(m_speed - newSpeed) < 0.0001)
			return false;

		m_speed = newSpeed;
	}
	else if (dir == DIR_DN)
	{
		if (m_speed <= MIN_SPEED)
			return false;
		m_speed /= SPEED_SCALE_FACTOR;
	}
	else
	{
		double newSpeed(1.0);
		if (newSpeed > getMaxSpeed())
			newSpeed = getMaxSpeed();

		if (fabs(m_speed - newSpeed) < 0.0001)
			return false;

		m_speed = newSpeed;
	}

	updateStatus();
	redrawStatus();
	return true;
}

void CView::updateStatus()
{
	if (!m_res.statusImage)
		return;

	std::string ts("?");
	if (m_lastTs > 0)
	{
		// nsec not used, not needed
		const time_t sec(m_lastTs >> 32);
		// const uint32_t nsec((m_lastTs & ((1L << 32) - 1)) % 1000);
		struct tm *tm(gmtime(&sec));

		char buf[256];
		snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);

		ts = buf;
	}

	std::string hz("?");
	std::string dbfs("?");
	if (m_mousex != -1 && m_mousex < (int) m_width)
	{
		if (m_lastRate != 0 && m_width > 1)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "%u", (m_mousex + m_shift) * m_lastRate / ((m_width * m_zoom - 1) * 2));
			hz = buf;
		}

		if (m_lastData.size() == m_width)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "-%u.%02u",
				m_lastData[m_mousex] / 100,
				m_lastData[m_mousex] % 100);
			dbfs = buf;
		}
	}

	std::string freeze("");
	if (m_freeze)
		freeze = " | Hold";

	char buf[256];
	snprintf(buf, sizeof(buf), "%s | W: %s | Spd: %.2f Z: %ux (%u - %u Hz) | M: %s Hz, %s dBFS%s",
		ts.c_str(), m_lastFftWindowName.c_str(), m_speed,
		m_zoom, m_shift * m_lastRate / ((m_width * m_zoom - 1) * 2),
		(m_shift + m_width - 1) * m_lastRate / ((m_width * m_zoom - 1) * 2),
		hz.c_str(), dbfs.c_str(), freeze.c_str());

	memset(m_res.statusImage->data, 0, m_width * STATUS_HEIGHT * 4);
	for (int x(0); buf[x]; ++x)
		drawStatusChar(x, buf[x]);
}

void CView::drawStatusChar(unsigned x, char ch)
{
	xassert(m_res.statusImage, "drawStatusChar() called without status bar image");
	for (unsigned y(0); y < font::CHAR_HEIGHT; ++y)
	{
		char line[font::CHAR_WIDTH];
		font::getLine(line, ch, y);

		for (unsigned cx(0); cx < font::CHAR_WIDTH; ++cx)
		{
			if (line[cx] && (((x * font::CHAR_WIDTH) + cx) < m_width))
			{
				const unsigned ofs(((y * m_width) + x * font::CHAR_WIDTH + cx) * 4);
				m_res.statusImage->data[ofs + 1] = 0xff;
			}
		}
	}
}

void CView::drawSignal()
{
	if (!m_res.mainImage)
		return;

	clearSignal();

	if (m_width != m_lastData.size() || m_width < 2)
		return;

	for (unsigned x(1); x < m_width; ++x)
	{
		unsigned y1(m_lastData[x - 1] * (m_signalHeight - 1) / SIGNAL_DBFS_MAX);
		unsigned y2(m_lastData[x] * (m_signalHeight - 1) / SIGNAL_DBFS_MAX);

		if (y1 > m_signalHeight - 1)
			y1 = m_signalHeight - 1;

		if (y2 > m_signalHeight - 1)
			y2 = m_signalHeight - 1;

		drawSignalLine(x - 1, y1, y2);
	}
}

void CView::updateWaterfall()
{
	if (!m_res.mainImage || m_width != m_lastData.size() || m_waterfallHeight < 2)
		return;

	for (unsigned y(1); y < m_waterfallHeight; ++y)
	{
		const unsigned ofsOld((y + m_signalHeight - 1) * m_width * 4);
		const unsigned ofsCur((y + m_signalHeight) * m_width * 4);
		memcpy(m_res.mainImage->data + ofsOld, m_res.mainImage->data + ofsCur, m_width * 4);
	}

	unsigned ofs((m_signalHeight + m_waterfallHeight - 1) * m_width * 4);
	for (unsigned x(0); x < m_width; ++x)
	{
		uint8_t bgr[3];
		soxpal::dbfsToRgb(bgr, m_lastData[x]);
		memcpy(m_res.mainImage->data + ofs, bgr, 3);
		ofs += 4;
	}
}

void CView::clearSignal()
{
	memset(m_res.mainImage->data, 0x00, m_width * m_signalHeight * 4);
}

void CView::drawSignalLine(unsigned x1, unsigned y1, unsigned y2)
{
	// in reality, we want to draw vertical stripes
	if (y1 == y2)
	{
		// special case: draw horizontal line
		drawSignalVertStripe(x1, y1);
		drawSignalVertStripe(x1 + 1, y1);
		return;
	}

	int xstart(x1), xend(x1 + 1);
	unsigned ystart(y1), yend(y2);

	if (y1 > y2)
	{
		xstart = x1 + 1;
		xend = x1;
		ystart = y2;
		yend = y1;
	}

	const double slope((double) (xend - xstart) / (yend - ystart));
	for (unsigned y(ystart); y <= yend; ++y)
	{
		const unsigned x(xstart + (y - ystart) * slope);
		drawSignalVertStripe(x, y);
	}
}

void CView::drawSignalVertStripe(unsigned x, unsigned ystart)
{
	uint8_t bgr[3] = { 0xff, 0xff, 0xff };
	drawSignalPixel(x, ystart, bgr);

	for (unsigned y(ystart + 1); y < (m_signalHeight - 1); ++y)
	{
		bgr[0] = (y * 0xc0 / (m_signalHeight - 1)) ^ 0xff;
		bgr[1] = bgr[2] = (y * 0xff / (m_signalHeight - 1)) ^ 0xff;
		for (unsigned i(0); i < 3; ++i)
			bgr[i] = (unsigned) bgr[i] * (m_signalHeight * 2 - y - 1) / (m_signalHeight * 2 - 1);
		drawSignalPixel(x, y, bgr);
	}
}

void CView::drawSignalPixel(unsigned x, unsigned y, const uint8_t bgr[])
{
	const unsigned ofs(((y * m_width) + x) * 4);
	memcpy(m_res.mainImage->data + ofs, bgr, 3);
}

double CView::getMaxSpeed() const
{
	// number of samples: sample rate / speed
	// fft width: m_width * 2
	// number of samples must be >= fft width

	if (m_lastRate == 0 || m_width == 0)
	{
		// cannot calculate
		return 1.0;
	}

	return (double) m_lastRate / (m_width * 2 * m_zoom);
}

void CView::updateFreeze()
{
	m_freeze = !m_freeze;
	updateStatus();
	redrawStatus();
}

bool CView::updateZoom(bool up)
{
	if (up)
	{
		++m_zoom;
		if (m_speed > getMaxSpeed())
			if (updateSpeed(DIR_UP))
			{
				// no need to call updateStatus() and redrawStatus() now
				// because updateSpeed() did it for us
				return true;
			}
	}
	else
	{
		if (m_zoom == 1)
			return false;

		--m_zoom;
		if (m_shift > getMaxShift())
			m_shift = getMaxShift();
	}

	updateStatus();
	redrawStatus();

	return true;
}

void CView::updateShift(bool up)
{
	const unsigned maxShift(getMaxShift());
	unsigned step(m_width * m_zoom / SHIFT_STEP_DIVISOR);
	if (step == 0)
		step = 1;
	else if (step > m_width / 2)
		step = m_width / 2;

	if (!up)
	{
		if (m_shift == 0)
			return;

		if (m_shift > step)
			m_shift -= step;
		else
			m_shift = 0;
	}
	else
	{
		if (m_shift >= maxShift)
			return;

		if ((m_shift + step) >= maxShift)
			m_shift = maxShift;
		else
			m_shift += step;
	}

	updateStatus();
	redrawStatus();
}

bool CView::resetZoomAndShift()
{
	if (m_zoom == 1 && m_shift == 0)
		return false;

	const bool rs(m_zoom != 1);

	m_zoom = 1;
	m_shift = 0;

	updateStatus();
	redrawStatus();

	return rs;
}

unsigned CView::getMaxShift()
{
	const unsigned range(m_width * m_zoom);
	if (range == 0 || m_zoom == 1)
		return 0;

	return range - m_width - 1;
}
