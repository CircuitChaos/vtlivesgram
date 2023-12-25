#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/select.h>
#include "source.h"
#include "view.h"
#include "throw.h"
#include "cli.h"

static int main2(int argc, char *const argv[])
{
	Cli cli(argc, argv);
	if(cli.shouldExit()) {
		return EXIT_SUCCESS;
	}

	View view;
	Source src(view.getWidth(), view.getSpeed(), cli.getSampleRate());

	const int xfd(view.getFd());
	const int sfd(src.getFd());
	int nfd(((xfd > sfd) ? xfd : sfd) + 1);
	bool inputEof(false);

	for(;;) {
		fd_set rfd;
		FD_ZERO(&rfd);
		FD_SET(xfd, &rfd);

		if(!inputEof) {
			FD_SET(sfd, &rfd);
		}

		const int rs(select(nfd, &rfd, NULL, NULL, NULL));
		if(rs == -1 && errno == EINTR) {
			continue;
		}

		xassert(rs >= 0, "select(): %m");
		xassert(rs > 0, "select(): returned zero");

		// we don't check for readability of xfd, because when doing fast window
		// updates (very fast speed), for some reason select() stops returning
		// its readibility. we'll rely on sfd readibility instead. not the best
		// solution, but I don't have any better one. there's XPending() function
		// (in view.evt()) anyway, so it won't hang.
		const uint32_t e(view.evt());

		if(e & View::EVT_TERMINATE) {
			break;
		}

		if(e & View::EVT_CONFIG_CHANGED) {
			src.setWidth(view.getWidth());
			src.setSpeed(view.getSpeed());
		}

		if(e & View::EVT_NEXT_FFT_WINDOW) {
			src.nextWindow();
		}

		if(!inputEof && FD_ISSET(sfd, &rfd)) {
			std::vector<std::vector<uint16_t>> data;
			uint32_t rate;
			uint64_t ts;
			std::string window;

			if(!src.read(data, rate, ts, window)) {
				if(cli.getWaitOnEof()) {
					inputEof = true;
					nfd      = xfd + 1;
				}
				else {
					break;
				}
			}

			for(std::vector<std::vector<uint16_t>>::const_iterator i(data.begin()); i != data.end(); ++i) {
				view.update(*i, rate, ts, window);
			}
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *const argv[])
{
	try {
		return main2(argc, argv);
	}
	catch(const std::runtime_error &e) {
		fprintf(stderr, "Fatal error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
