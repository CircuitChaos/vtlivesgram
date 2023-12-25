#include <unistd.h>
#include "source.h"
#include "throw.h"
#include "vtparser.h"
#include "rawparser.h"

Source::Source(unsigned initialWidth, double initialSpeed, uint32_t sampleRate)
    : m_width(initialWidth),
      m_speed(initialSpeed)
{
	if(!sampleRate) {
		m_parser.reset(new VtParser());
	}
	else {
		m_parser.reset(new RawParser(sampleRate));
	}
}

int Source::getFd() const
{
	return 0;
}

void Source::setWidth(unsigned width)
{
	m_width = width;
}

void Source::setSpeed(double speed)
{
	m_speed = speed;
}

void Source::nextWindow()
{
	m_fft.nextWindow();
}

bool Source::read(std::vector<std::vector<uint16_t>> &data, uint32_t &rate, uint64_t &ts, std::string &fftWindowName)
{
	data.clear();
	rate = 0;
	ts   = 0;

	uint8_t buf[8192];
	const ssize_t rs(::read(0, buf, sizeof(buf)));
	if(rs == 0)
		return false;

	xassert(rs > 0, "read(): %m");
	xassert(rs <= (ssize_t) sizeof(buf), "read(): returned nonsense");

	fftWindowName = m_fft.getWindowName();

	m_parser->feed(buf, rs);

	if(!m_parser->isInitialized())
		return true;

	const uint32_t numSamples(m_parser->getRate() / m_speed);
	while(m_parser->count() >= numSamples) {
		ts   = m_parser->getLastTs();
		rate = m_parser->getRate();

		std::vector<double> in;
		in.resize(numSamples);
		m_parser->read(&in[0], numSamples);

		std::vector<uint16_t> out;
		out.resize(m_width);
		m_fft(out, in);

		data.push_back(out);
	}

	return true;
}
