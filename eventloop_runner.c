#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include <poll.h>

#include "eventloop_runner.h"
#include "testcase_runner.h"

enum { NUM_POLL_FDS = 32 };
struct pollfd pollfds[NUM_POLL_FDS];

int eventloop_run;

void log_poll_events(struct pollfd pfd)
{
    if (pfd.revents & POLLIN) {
        fprintf(stderr, "fd %d readable\n", pfd.fd);
    }

    if (pfd.revents & POLLOUT) {
        fprintf(stderr, "fd %d writeable\n", pfd.fd);
    }

    if (pfd.revents & POLLPRI) {
        fprintf(stderr, "fd %d priority readable\n", pfd.fd);
    }

    if (pfd.revents & POLLERR) {
        fprintf(stderr, "fd %d output error\n", pfd.fd);
    }

    if (pfd.revents & POLLHUP) {
        fprintf(stderr, "fd %d hung up\n", pfd.fd);
    }

    if (pfd.revents & POLLNVAL) {
        fprintf(stderr, "fd %d not open\n", pfd.fd);
    }
}

void eventloop_start(struct test *tc)
{
    int rc;

    eventloop_run = 1;

    while (eventloop_run) {
        int i;
        int nfds = NUM_POLL_FDS;
        int timeout;
        struct timeval now;

        gettimeofday(&now, NULL);
        libxl_osevent_beforepoll(tc->ctx, &nfds, pollfds, &timeout, now);
        rc = poll(pollfds, NUM_POLL_FDS, 2000);

        if (rc == 0) {
            send_eventloop_timeout(tc);
        }

        for (;;) {
            libxl_event *event;
            rc = libxl_event_check(tc->ctx, &event, LIBXL_EVENTMASK_ALL, 0,
                                   0);
            if (rc == ERROR_NOT_READY) {
                printf("libxl_event_check found no events\n");
                break;
            }
            send_libxl_event(tc, event->type);
            libxl_event_free(tc->ctx, event);
        }

        for (i = 0; i < NUM_POLL_FDS; i++) {
            if (pollfds[i].fd != -1 && pollfds[i].revents != 0) {
                struct pollfd *pfd = &pollfds[i];
                log_poll_events(*pfd);
                send_fd_event(tc, pfd->fd);
            }
        }

        gettimeofday(&now, NULL);
        libxl_osevent_afterpoll(tc->ctx, nfds, pollfds, now);
    }
}

void eventloop_halt()
{
    eventloop_run = 0;
}
