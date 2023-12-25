#pragma once

#include <vector>
#include <inttypes.h>
#include <fftw3.h>

class Fft {
public:
	~Fft();

	void nextWindow();
	const char *getWindowName() const;

	// input: any number of samples (depending on the waterfall speed)
	// output: magnitudes in dbFS (output buffer already resized to the FFT width / 2)
	void operator()(std::vector<uint16_t> &out, const std::vector<double> &in);

private:
	enum WindowType {
		WIN_HANNING,
		WIN_RECT,
		WIN_COSINE,
		WIN_HAMMING,
		WIN_BLACKMAN,
		WIN_NUTTALL,

		// must be last
		WIN_MAX,
	};

	WindowType m_windowType{WIN_NUTTALL};
	unsigned m_width{0};
	double *m_window;
	double *m_input;
	fftw_complex *m_output;
	fftw_plan m_plan;

	void init(unsigned width);
	void createWindow();
	double windowFunction(unsigned i) const;
};
