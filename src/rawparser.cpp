#include "rawparser.h"

RawParser::RawParser(uint32_t sampleRate)
    : m_sampleRate(sampleRate)
{
}

void RawParser::feed(const uint8_t *buf, size_t sz)
{
	for(size_t i(0); i < sz; ++i) {
		feedByte(buf[i]);
	}
}

void RawParser::feedByte(uint8_t byte)
{
	if(m_strayByte == 0x100) {
		m_strayByte = byte;
		return;
	}

	const uint8_t byte1(m_strayByte);
	const uint8_t byte2(byte);
	m_strayByte = 0x100;

	const uint16_t uword((byte2 << 8) | byte1);
	const int16_t sword((int16_t) uword);

	feedSample((double) sword / (INT16_MAX + 1));
}
