#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <sys/select.h>
#include "source.h"
#include "view.h"
#include "throw.h"

static int main2(int argc, char * const argv[])
{
	double initialSpeed(1.0);

	if (argc > 2)
	{
		printf("Syntax: vtcat ... | vtlivesgram [<speed>]\n");
		printf("Initial speed is in pixels per second\n");
		return EXIT_SUCCESS;
	}

	if (argc > 1)
		initialSpeed = atof(argv[1]);

	CView view(initialSpeed);
	CSource src(view.getWidth(), view.getSpeed());

	const int xfd(view.getFd());
	const int sfd(src.getFd());
	const int nfd(((xfd > sfd) ? xfd : sfd) + 1);

	for (;;)
	{
		fd_set rfd;
		FD_ZERO(&rfd);
		FD_SET(xfd, &rfd);
		FD_SET(sfd, &rfd);

		const int rs(select(nfd, &rfd, NULL, NULL, NULL));
		if (rs == -1 && errno == EINTR)
			continue;

		xassert(rs >= 0, "select(): %m");
		xassert(rs > 0, "select(): returned zero");

		if (FD_ISSET(xfd, &rfd))
		{
			const uint32_t e(view.evt());

			if (e & CView::EVT_TERMINATE)
				break;

			if (e & CView::EVT_SIZE_CHANGED)
				src.setWidth(view.getWidth());

			if (e & CView::EVT_SPEED_CHANGED)
				src.setSpeed(view.getSpeed());
		}

		if (FD_ISSET(sfd, &rfd))
		{
			std::vector<std::vector<uint16_t> > data;
			uint32_t rate;
			uint64_t ts;

			if (!src.read(data, rate, ts))
				break;

			for (std::vector<std::vector<uint16_t> >::const_iterator i(data.begin()); i != data.end(); ++i)
				view.update(*i, rate, ts);
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char * const argv[])
{
	try
	{
		return main2(argc, argv);
	}
	catch (const std::runtime_error &e)
	{
		fprintf(stderr, "Fatal error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
