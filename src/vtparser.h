#pragma once

#include <ctime>
#include <inttypes.h>
#include <vector>

class CVtParser
{
public:
	CVtParser();

	bool isInitialized() const;

	uint32_t getRate() const;
	uint32_t count() const;
	uint64_t getLastTs() const;

	void read(double *samples, uint32_t count);
	void feed(const uint8_t *buf, size_t sz);

private:
	bool m_initialized;
	uint32_t m_rate;
	uint64_t m_lastTs;
	uint32_t m_curSample;
	bool m_inData;

	// copied and adapted from vtlib.h
	struct SHdr
	{
		int magic;
		uint32_t flags;		// See VT_FLAG_*
		uint32_t bsize;		// Number of frames per block
		uint32_t chans;		// Number of channels per frame
		uint32_t sample_rate;	// Nominal sample rate
		uint32_t secs;		// Timestamp, seconds part
		uint32_t nsec;		// Timestamp, nanoseconds part
		int32_t valid;		// Set to one if block is valid
		int32_t frames;		// Number of frames actually contained
		int32_t spare;
		double srcal;		// Actual rate is sample_rate * srcal
	} __attribute__((packed));

	SHdr m_hdr;
	std::vector<uint8_t> m_buf;
	std::vector<double> m_data;

	void feedHeaderByte(uint8_t byte);
	void feedDataByte(uint8_t byte);
};
