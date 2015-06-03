#include <libxl_event.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/select.h>

#include "xl_eventloop.h"
#include "thread_test.h"
#include "testcase.h"

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	struct test t;
	xentoollog_logger *logger;

	init_pollfds(pollfds, NUM_POLL_FDS);

	logger = (xentoollog_logger*)
		xtl_createlogger_stdiostream(stderr, XTL_PROGRESS,  0);

	if (!!test_spawn(&t, logger, testcase)) {
		perror("Failed to spawn test thread");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int i, rc;

		rc = poll(pollfds, NUM_POLL_FDS, 2000);
		if (rc == 0) {
			send_eventloop(&t);
			continue;
		}

		for (i = 0; i < NUM_POLL_FDS; i++) {

			if (pollfds[i].revents & POLLIN) {
				fprintf(stderr, "fd %d readable\n", pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLOUT) {
				fprintf(stderr, "fd %d writeable\n", pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLPRI) {
				fprintf(stderr, "fd %d priority readable\n", pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLERR) {
				fprintf(stderr, "fd %d output error\n", pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLHUP) {
				fprintf(stderr, "fd %d hung up\n", pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLNVAL) {
				fprintf(stderr, "fd %d not open\n", pollfds[i].fd);
			}

			if (pollfds[i].revents != 0) {
				struct libxl_task *lxt = &libxl_tasks[i];
				struct pollfd *pfd = &pollfds[i];

				libxl_osevent_occurred_fd(lxt->task->ctx, lxt->for_libxl,
					pfd->fd, pfd->events, pfd->revents);
				send_fd_event(&t, pfd->fd);
			}
		}
        }

	test_destroy(&t);
	xtl_logger_destroy(logger);
        exit(EXIT_SUCCESS);
}
