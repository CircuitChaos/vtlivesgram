#pragma once

#include <ctime>
#include <inttypes.h>
#include <vector>

class InputParser {
public:
	void read(double *samples, uint32_t count);
	uint32_t count() const;

	virtual void feed(const uint8_t *buf, size_t sz) = 0;
	virtual bool isInitialized() const               = 0;
	virtual uint32_t getRate() const                 = 0;
	virtual uint64_t getLastTs() const               = 0;

protected:
	void feedSample(double sample);

private:
	std::vector<double> m_samples;
};
