#pragma once

#include <vector>
#include <string>
#include <inttypes.h>
#include <memory>
#include "inputparser.h"
#include "fft.h"

class Source {
public:
	// sampleRate = 0: use vt input format
	// sampleRate > 0: use raw input format (mono, s16_le)
	Source(unsigned initialWidth, double initialSpeed, uint32_t sampleRate);

	// this is the fd of stdin
	int getFd() const;

	void setWidth(unsigned width);
	void setSpeed(double speed);
	void nextWindow();

	// - call it after getFd() returns readability
	// - data in -dBFS * 100 (0 = max, 12000 = -120 dBFS)
	// - might be more than one line returned
	// - ts and rate valid only if data not empty
	// - returns false on EOF, throws on error
	bool read(std::vector<std::vector<uint16_t>> &data, uint32_t &rate, uint64_t &ts, std::string &fftWindowName);

private:
	unsigned m_width;
	double m_speed;
	std::unique_ptr<InputParser> m_parser;
	Fft m_fft;
};
