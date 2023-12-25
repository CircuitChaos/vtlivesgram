#pragma once

#include "inputparser.h"

class RawParser : public InputParser {
public:
	RawParser(uint32_t sampleRate);

	virtual bool isInitialized() const final { return true; }
	virtual uint32_t getRate() const final { return m_sampleRate; }
	virtual uint64_t getLastTs() const final { return 0; }
	virtual void feed(const uint8_t *buf, size_t sz) final;

private:
	const uint32_t m_sampleRate;
	uint16_t m_strayByte{0x100};

	void feedByte(uint8_t byte);
};
