#pragma once

#include <vector>
#include <inttypes.h>
#include "vtparser.h"
#include "fft.h"

class CSource
{
public:
	CSource(unsigned initialWidth, double initialSpeed);
	~CSource();

	// this is the fd of stdin
	int getFd() const;

	void setWidth(unsigned width);
	void setSpeed(double speed);

	// - call it after getFd() returns readability
	// - data in -dBFS * 100 (0 = max, 12000 = -120 dBFS)
	// - might be more than one line returned
	// - ts and rate valid only if data not empty
	// - returns false on EOF, throws on error
	bool read(std::vector<std::vector<uint16_t> > &data, uint32_t &rate, uint64_t &ts);

private:
	unsigned m_width;
	double m_speed;
	CVtParser m_vt;
	CFft m_fft;
};
