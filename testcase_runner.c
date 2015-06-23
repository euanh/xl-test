#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "testcase_runner.h"
#include "eventloop_runner.h"

enum { RX = 0, TX = 1 };

int
test_spawn(struct test *tc, xentoollog_logger * logger,
           void *(*fn) (struct test *))
{
    /* Initialize the test structure */
    libxl_ctx_alloc(&tc->ctx, LIBXL_VERSION, 0, logger);

    /* Initialize the mailbox */
    if (pipe(tc->mailbox) != 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* Spawn a thread to run the test case */
    return pthread_create(&tc->thread, NULL, (void *(*)(void *))fn, tc);
}

void test_join(struct test *tc)
{
    /* we should capture and return retval here */
    pthread_join(tc->thread, NULL);
}

void test_destroy(struct test *tc)
{
    if (tc->mailbox[RX] >= 0) {
        close(tc->mailbox[RX]);
    }

    if (tc->mailbox[TX] >= 0) {
        close(tc->mailbox[TX]);
    }

    libxl_ctx_free(tc->ctx);
}

void test_exit()
{
    eventloop_halt();
    pthread_exit(NULL);
}

int send_event(struct test *tc, struct event ev)
{
    return write(tc->mailbox[TX], (char *)&ev, sizeof(ev));
}

void recv_event(struct test *tc, struct event *ev)
{
    printf("waiting for event\n");
    read(tc->mailbox[RX], (char *)ev, sizeof(*ev));
}

/* Wait until the an event matching the expected mask is posted.
   Ignores and discards any non-matching events received in the meantime. */
void wait_for(struct test *tc, enum event_type mask, struct event *ev)
{
    do {
        recv_event(tc, ev);
    } while (!(ev->type & mask));
}

/* Wait until the the specified number of the expected events are posted.
   Ignores and discards any other events received in the meantime. */
void
wait_for_n(struct test *tc, enum event_type mask, int count, struct event *ev)
{
    while (count--) {
        wait_for(tc, mask, ev);
    }
}

/* Wait until an event matching the mask is posted, or count other events
   are posted.  Contiguous runs of up to batch_size of the same event are 
   counted as 1 event.
   Returns 1 if the matching event is posted, 0 otherwise.
 */
bool wait_until_n(struct test *tc, enum event_type mask, int count,
                 struct event *ev, int batch_size)
{
    enum event_type prev_event = EV_NONE;

    while (count--) {
        wait_for(tc, ~EV_EVENTLOOP, ev);
        if (ev->type & mask) {
            return 1;
        }

        if ((batch_size > 0) && (ev->type == prev_event)) {
            count++;
        }

        prev_event = ev->type;
    }
    return 0;
}

int send_fd_event(struct test *tc, int fd)
{
    struct event ev;
    ev.type = EV_FD_EVENT;
    ev.u.fd_event.fd = fd;
    printf("-> EV_FD_EVENT\n");
    return send_event(tc, ev);
}

int send_libxl_event(struct test *tc, libxl_event_type type)
{
    struct event ev;
    ev.type = EV_LIBXL_EVENT;
    ev.u.libxl_event.type = type;
    printf("-> EV_LIBXL_EVENT");
    return send_event(tc, ev);
}

int send_libxl_callback_event(struct test *tc, int rc)
{
    struct event ev;
    ev.type = EV_LIBXL_CALLBACK;
    ev.u.callback_event.rc = rc;
    printf("-> EV_LIBXL_CALLBACK\n");
    return send_event(tc, ev);
}

int send_eventloop_timeout(struct test *tc)
{
    struct event ev;
    ev.type = EV_EVENTLOOP;
    printf("-> EV_EVENTLOOP_TICK\n");
    return send_event(tc, ev);
}
