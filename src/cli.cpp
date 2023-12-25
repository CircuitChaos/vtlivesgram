#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include "throw.h"
#include "cli.h"

Cli::Cli(int argc, char *const argv[])
{
	int opt;
	while((opt = getopt(argc, argv, ":hr:s:w")) != -1 && !m_shouldExit) {
		switch(opt) {
			case '?':
				xthrow("Option -%c not recognized", optopt);

			case ':':
				xthrow("Option -%c requires argument", optopt);

			case 'h':
				help();
				m_shouldExit = true;
				break;

			case 'r':
				m_sampleRate = atoi(optarg);
				break;

			case 's':
				m_initialSpeed = atof(optarg);
				break;

			case 'w':
				m_waitOnEof = true;
				break;

			default:
				xthrow("Unknown return value from getopt: %d", opt);
		}
	}
}

void Cli::help()
{
	static const char helpstr[] =
	    "Syntax to read from vlfrx-tools:\n"
	    "  vtcat ... | vtlivesgram [options]\n"
	    "\n"
	    "Syntax to read raw, mono, s16_le stream:\n"
	    "  arecord -t raw -f s16_le -c 1 -r <rate> ... | vtlivesgram -r <rate> [options]\n"
	    "\n"
	    "Options:\n"
	    "  -h: show help\n"
	    "  -r <rate>: specify sample rate\n"
	    "  -s <speed>: specify initial speed\n"
	    "  -w: wait on EOF\n"
	    "\n"
	    "If sample rate is specified, then input type is changed from vt \n"
	    "to raw, mono, s16_le.";

	puts(helpstr);
}
