#include <cstring>
#include "vtparser.h"
#include "throw.h"

// copied and adapted from vtlib.h
static const int VT_BLOCK_MAGIC	= 27859;

#define VT_FLAG_RELT	(1<<0)	// Timestamps are relative, not absolute
#define VT_FLAG_FLOAT4	(1<<1)	// 4 byte floats  (8 byte default)
#define VT_FLAG_FLOAT8	0
#define VT_FLAG_INT1	(2<<1)	// 1 byte signed integers
#define VT_FLAG_INT2	(3<<1)	// 2 byte signed integers
#define VT_FLAG_INT4	(4<<1)	// 4 byte signed integers
#define VT_FLAG_FMTMASK	(VTFLAG_FLOAT4 | VTFLAG_INT1 | VTFLAG_INT2 | VTFLAG_INT4)	// Mask for format bits

CVtParser::CVtParser():
	m_initialized(false),
	m_rate(0),
	m_lastTs(0),
	m_curSample(0),
	m_inData(false)
{
}

bool CVtParser::isInitialized() const
{
	return m_initialized;
}

uint32_t CVtParser::getRate() const
{
	return m_rate;
}

uint32_t CVtParser::count() const
{
	return m_data.size();
}

uint64_t CVtParser::getLastTs() const
{
	return m_lastTs;
}

void CVtParser::feed(const uint8_t *buf, size_t sz)
{
	xassert(sz > 0, "Trying to feed empty data");

	// it's not the most efficient way -- to be rewritten
	while (sz--)
		if (m_inData)
			feedDataByte(*buf++);
		else
			feedHeaderByte(*buf++);
}

void CVtParser::read(double *samples, uint32_t count)
{
	xassert(m_data.size() >= count, "Trying to read too many samples");
	xassert(count > 0, "Trying to read zero samples");
	memcpy(samples, &m_data[0], count * sizeof(double));
	m_data.erase(m_data.begin(), m_data.begin() + count);
}

void CVtParser::feedHeaderByte(uint8_t byte)
{
	m_buf.push_back(byte);
	if (m_buf.size() < sizeof(m_hdr))
		return;

	memcpy(&m_hdr, &m_buf[0], sizeof(m_hdr));
	m_buf.clear();

	xassert(m_hdr.magic == VT_BLOCK_MAGIC, "VT: invalid block magic");
	xassert(m_hdr.flags == 0, "VT: not implemented flags combination (0x%08x), contact author", m_hdr.flags);
	xassert(m_hdr.chans == 1, "VT: only one channel in input stream is supported (got %u)", m_hdr.chans);
	xassert(m_hdr.sample_rate > 0, "VT: invalid sample rate");
	xassert(m_hdr.valid == 1, "VT: invalid frame received");
	xassert(m_hdr.frames != 0, "VT: received frame with no data");
	xassert((int32_t) m_hdr.bsize >= m_hdr.frames, "VT: block header size %u less than frame count %d", m_hdr.bsize, m_hdr.frames);

	if (!m_initialized)
	{
		m_rate = m_hdr.sample_rate;
		m_initialized = true;
	}
	else
		xassert(m_rate == m_hdr.sample_rate, "VT: sample rate suddenly changed");

	m_lastTs = ((uint64_t) m_hdr.secs << 32) | m_hdr.nsec;
	m_curSample = 0;
	m_inData = true;
}

void CVtParser::feedDataByte(uint8_t byte)
{
	m_buf.push_back(byte);
	if (m_buf.size() < sizeof(double))
		return;

	double sample;
	memcpy(&sample, &m_buf[0], sizeof(sample));
	m_buf.clear();

	if ((int32_t) m_curSample < m_hdr.frames)
		m_data.push_back(sample);

	if (++m_curSample == m_hdr.bsize)
		m_inData = false;
}
