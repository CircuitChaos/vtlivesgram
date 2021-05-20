#pragma once

#include <vector>
#include <inttypes.h>
#include "xresources.h"

class CView
{
public:
	enum EEvt
	{
		EVT_NONE		= 0,

		EVT_TERMINATE		= 1 << 0,
		EVT_WIDTH_CHANGED	= 1 << 1,
		EVT_SPEED_CHANGED	= 1 << 2,
		EVT_NEXT_FFT_WINDOW	= 1 << 3,
	};

	CView(double initialSpeed);
	~CView();

	int getFd() const;

	unsigned getWidth() const;
	double getSpeed() const;

	uint32_t evt();
	void update(const std::vector<uint16_t> &data, uint32_t rate, uint64_t ts, const std::string &fftWindowName);

private:
	int m_fd;
	unsigned m_width;
	unsigned m_height;
	double m_speed;
	int m_mousex;
	uint32_t m_lastRate;
	uint64_t m_lastTs;
	std::vector<uint16_t> m_lastData;
	std::string m_lastFftWindowName;
	SXResources m_res;
	unsigned m_signalHeight;
	unsigned m_waterfallHeight;
	bool m_freeze;

	enum EDir
	{
		DIR_UP,
		DIR_DN,
		DIR_RESET,
	};

	void redrawStatus();
	void redrawAll();
	void resetImages();
	void updateMouse(unsigned x, unsigned y);
	bool updateSpeed(EDir dir);
	void updateStatus();
	void drawStatusChar(unsigned x, char ch);
	void clearSignal();
	void drawSignal();
	void updateWaterfall();
	// draws line from x1,y1 to x1+1,y2
	void drawSignalLine(unsigned x1, unsigned y1, unsigned y2);
	void drawSignalVertStripe(unsigned x, unsigned ystart);
	void drawSignalPixel(unsigned x, unsigned y, const uint8_t bgr[3]);
	double maxSpeed() const;
};
