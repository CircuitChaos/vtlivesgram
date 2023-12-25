#pragma once

#include <inttypes.h>

class Cli {
public:
	Cli(int argc, char *const argv[]);

	bool shouldExit() const { return m_shouldExit; }
	bool getWaitOnEof() const { return m_waitOnEof; }
	uint32_t getSampleRate() const { return m_sampleRate; }
	double getInitialSpeed() const { return m_initialSpeed; }
	unsigned getInitialZoom() const { return m_initialZoom; }
	unsigned getInitialShift() const { return m_initialShift; }

private:
	bool m_shouldExit{false};
	bool m_waitOnEof{false};
	uint32_t m_sampleRate{0};
	double m_initialSpeed{1.0};
	unsigned m_initialZoom{1};
	unsigned m_initialShift{0};

	void help();
};
