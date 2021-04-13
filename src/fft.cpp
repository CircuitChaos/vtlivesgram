#include <cstring>
#include <math.h>
#include "throw.h"
#include "fft.h"

CFft::CFft():
	m_windowType(WIN_HANNING),
	m_width(0)
{
}

CFft::~CFft()
{
	init(0);
	fftw_cleanup();
}

void CFft::operator()(std::vector<uint16_t> &out, const std::vector<double> &in)
{
	// if width has changed, we need to reinitialize the plan and window
	const unsigned width(out.size() * 2);
	if (width != m_width)
		init(width);

	// for example:
	// - out size is 10
	// - width is 20
	// - number of samples is 45
	//
	// we do:
	// - one fft for samples 0...19
	// - one fft for samples 20...39
	// - we ignore samples 40...44 for now (we'll see if it's better or worse)
	//
	// maybe better approach would be to use also remaining samples, applying
	// a reduced-size window on them and padding them with zeroes? we'll see.

	std::vector<double> magnitudes;
	magnitudes.resize(width / 2);
	std::fill(magnitudes.begin(), magnitudes.end(), 0.0);

	for (unsigned i(0); (i + 1) * width <= in.size(); ++i)
	{
		memcpy(m_input, &in[i * width], width * sizeof(double));
		for (unsigned j(0); j < width; ++j)
			m_input[j] *= m_window[j];

		fftw_execute(m_plan);

		for (unsigned j(0); j < width / 2; ++j)
		{
			const double re(m_output[j][0]);
			const double im(m_output[j][1]);
			const double magnitude(sqrt(re * re + im * im));
			magnitudes[j] += magnitude;
		}
	}

	// i'm not sure what should be done with samples that are left
	// (in.size() % width samples)...

	unsigned iterations(in.size() / width);
	unsigned scaleFactor(width * iterations);

	// printf("w=%d insize=%d iterations=%d left=%d factor=%.2f\n",
	//	m_width, in.size(), iterations, in.size() % width, scaleFactor);

	for (unsigned i(0); i < width / 2; ++i)
	{
		double dbfs(20 * log10(magnitudes[i] / scaleFactor));

		if (dbfs > 0.0)
			dbfs = 0.0;
		else if (dbfs < -650.0)
			dbfs = -650.0;

		out[i] = dbfs * -100.0;
	}
}

void CFft::init(unsigned width)
{
	if (m_width)
	{
		delete[] m_output;
		delete[] m_input;
		delete[] m_window;
		fftw_destroy_plan(m_plan);
	}

	m_width = width;
	if (width == 0)
		return;

	m_window = new double[width];
	m_input = new double[width];
	m_output = new fftw_complex[width];

	createWindow();
	m_plan = fftw_plan_dft_r2c_1d(width, m_input, m_output, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
}

void CFft::nextWindow()
{
	// not the most elegant solution...
	int wnd(m_windowType);
	++wnd;
	if (wnd == WIN_MAX)
		wnd = 0;
	m_windowType = (EWindowType) wnd;
	createWindow();
}

const char *CFft::getWindowName() const
{
	const char *rs(NULL);

	switch (m_windowType)
	{
		case WIN_HANNING:
			rs = "Hanning";
			break;

		case WIN_RECT:
			rs = "rectangular";
			break;

		case WIN_COSINE:
			rs = "cosine";
			break;

		case WIN_HAMMING:
			rs = "Hamming";
			break;

		case WIN_BLACKMAN:
			rs = "Blackman";
			break;

		case WIN_NUTTALL:
			rs = "Nuttall";
			break;

		default:
			break;
	}

	xassert(rs, "Unknown FFT window %d", m_windowType);
	return rs;
}

void CFft::createWindow()
{
	for (unsigned i(0); i < m_width; ++i)
		m_window[i] = windowFunction(i);
}

double CFft::windowFunction(unsigned i) const
{
	const double di(i);
	switch (m_windowType)
	{
		case WIN_HANNING:
			return sin(di / m_width * M_PI) * sin(di / m_width * M_PI);

		case WIN_RECT:
			return 1;

		case WIN_COSINE:
			return sin(di / m_width * M_PI);

		case WIN_HAMMING:
			return 0.54 - 0.46 * cos(di / m_width * 2 * M_PI);

		case WIN_BLACKMAN:
		{
			const double a0((1 - 0.16) / 2);
			const double a1(0.5);
			const double a2(0.16 / 2);
			return a0
				- a1 * cos(di / m_width * 2 * M_PI)
				+ a2 * cos(di / m_width * 4 * M_PI);
		}

		case WIN_NUTTALL:
		{
			const double a0(0.355768);
			const double a1(0.487396);
			const double a2(0.144232);
			const double a3(0.012604);
			return a0
				- a1 * cos(di / m_width * 2 * M_PI)
				+ a2 * cos(di / m_width * 4 * M_PI)
				- a3 * cos(di / m_width * 6 * M_PI);
		}

		default:
			xthrow("Unknown FFT window %d", m_windowType);
			/* NOTREACHED */
			break;
	}

	return 0;
}
