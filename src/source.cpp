#include <unistd.h>
#include "source.h"
#include "throw.h"

CSource::CSource(unsigned initialWidth, double initialSpeed):
	m_width(initialWidth),
	m_speed(initialSpeed)
{
}

CSource::~CSource()
{
}

int CSource::getFd() const
{
	return 0;
}

void CSource::setWidth(unsigned width)
{
	m_width = width;
}

void CSource::setSpeed(double speed)
{
	m_speed = speed;
}

void CSource::nextWindow()
{
	m_fft.nextWindow();
}

bool CSource::read(std::vector<std::vector<uint16_t> > &data, uint32_t &rate, uint64_t &ts, std::string &fftWindowName)
{
	data.clear();
	rate = 0;
	ts = 0;

	// tbd: make this size sane
	uint8_t buf[8192];
	const ssize_t rs(::read(0, buf, sizeof(buf)));
	if (rs == 0)
		return false;

	xassert(rs > 0, "read(): %m");
	xassert(rs <= (ssize_t) sizeof(buf), "read(): returned nonsense");

	m_vt.feed(buf, rs);

	if (!m_vt.isInitialized())
		return true;

	const uint32_t numSamples(m_vt.getRate() / m_speed);
	while (m_vt.count() >= numSamples)
	{
		ts = m_vt.getLastTs();
		rate = m_vt.getRate();
		fftWindowName = m_fft.getWindowName();

		std::vector<double> in;
		in.resize(numSamples);
		m_vt.read(&in[0], numSamples);

		std::vector<uint16_t> out;
		out.resize(m_width);
		m_fft(out, in);

		data.push_back(out);
	}

	return true;
}
