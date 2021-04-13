#pragma once

#include <vector>
#include <inttypes.h>
#include <fftw3.h>

class CFft
{
public:
	CFft();
	~CFft();

	void operator()(std::vector<uint16_t> &out, const std::vector<double> &in);

private:
	unsigned m_width;
	double *m_window;
	double *m_input;
	fftw_complex *m_output;
	fftw_plan m_plan;

	void init(unsigned width);
};
