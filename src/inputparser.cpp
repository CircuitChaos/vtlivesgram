#include <cstring>
#include "inputparser.h"
#include "throw.h"

void InputParser::read(double *samples, uint32_t count)
{
	xassert(m_samples.size() >= count, "Trying to read too many samples");
	xassert(count > 0, "Trying to read zero samples");
	memcpy(samples, &m_samples[0], count * sizeof(double));
	m_samples.erase(m_samples.begin(), m_samples.begin() + count);
}

uint32_t InputParser::count() const
{
	return m_samples.size();
}

void InputParser::feedSample(double sample)
{
	m_samples.push_back(sample);
}
