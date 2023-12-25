#pragma once

#include <inttypes.h>

class Cli {
public:
	Cli(int argc, char *const argv[]);

	bool shouldExit() const { return m_shouldExit; }
	bool getWaitOnEof() const { return m_waitOnEof; }
	uint32_t getSampleRate() const { return m_sampleRate; }

private:
	bool m_shouldExit{false};
	bool m_waitOnEof{false};
	uint32_t m_sampleRate{0};

	void help();
};
