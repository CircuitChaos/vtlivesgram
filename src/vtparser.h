#pragma once

#include "inputparser.h"

class VtParser : public InputParser {
public:
	virtual bool isInitialized() const final;
	virtual uint32_t getRate() const final;
	virtual uint64_t getLastTs() const final;
	virtual void feed(const uint8_t *buf, size_t sz) final;

private:
	bool m_initialized{false};
	uint32_t m_rate{0};
	uint64_t m_lastTs{0};
	uint32_t m_curSample{0};
	bool m_inData{false};

	// copied and adapted from vtlib.h
	struct Hdr {
		int magic;
		uint32_t flags;       // See VT_FLAG_*
		uint32_t bsize;       // Number of frames per block
		uint32_t chans;       // Number of channels per frame
		uint32_t sample_rate; // Nominal sample rate
		uint32_t secs;        // Timestamp, seconds part
		uint32_t nsec;        // Timestamp, nanoseconds part
		int32_t valid;        // Set to one if block is valid
		int32_t frames;       // Number of frames actually contained
		int32_t spare;
		double srcal;         // Actual rate is sample_rate * srcal
	} __attribute__((packed));

	Hdr m_hdr;
	std::vector<uint8_t> m_buf;

	void feedHeaderByte(uint8_t byte);
	void feedDataByte(uint8_t byte);
};
