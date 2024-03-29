#pragma once

#include <vector>
#include <inttypes.h>
#include "xresources.h"

class View {
public:
	enum Evt {
		EVT_NONE = 0,

		EVT_TERMINATE       = 1 << 0,
		EVT_CONFIG_CHANGED  = 1 << 1,
		EVT_NEXT_FFT_WINDOW = 1 << 2,
	};

	View(double initialSpeed, unsigned initialZoom, unsigned initialShift);
	~View();

	int getFd() const;

	unsigned getWidth() const;
	double getSpeed() const;

	uint32_t evt();
	uint32_t update(const std::vector<uint16_t> &data, uint32_t rate, uint64_t ts, const std::string &fftWindowName);

private:
	int m_fd{-1};
	unsigned m_width{0};
	unsigned m_height{0};
	double m_speed;
	unsigned m_zoom;
	unsigned m_shift;
	int m_mousex{-1};
	uint32_t m_lastRate{0};
	uint64_t m_lastTs{0};
	std::vector<uint16_t> m_lastData;
	std::string m_lastFftWindowName{"?"};
	XResources m_res;
	unsigned m_signalHeight{0};
	unsigned m_waterfallHeight{0};
	bool m_freeze{false};

	enum Dir {
		DIR_UP,
		DIR_DN,
		DIR_RESET,
	};

	void redrawStatus();
	void redrawAll();
	void resetImages();
	void updateMouse(unsigned x, unsigned y);
	bool updateSpeed(Dir dir);
	void updateStatus();
	void drawStatusChar(unsigned x, char ch);
	void clearSignal();
	void drawSignal();
	void updateWaterfall();
	// draws line from x1,y1 to x1+1,y2
	void drawSignalLine(unsigned x1, unsigned y1, unsigned y2);
	void drawSignalVertStripe(unsigned x, unsigned ystart);
	void drawSignalPixel(unsigned x, unsigned y, const uint8_t bgr[3]);
	double getMaxSpeed() const;
	void updateFreeze();

	// updateZoom() with dir == UP can also update speed
	// updateZoom() with dir == DOWN can also update shift
	bool updateZoom(bool up);
	void updateShift(bool up);
	bool resetZoomAndShift();
	unsigned getMaxShift(); // returns max shift for current zoom and window size
};
