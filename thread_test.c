#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "thread_test.h"
#include "xl_eventloop.h"


int
test_spawn(struct test *tc, xentoollog_logger *logger,
           void *(*fn)(struct test *))
{
	/* Initialize the test structure */
	libxl_ctx_alloc(&tc->ctx, LIBXL_VERSION, 0, logger);
	register_callbacks(tc);

	/* Initialize the mailbox */
	pthread_mutex_init(&tc->mailbox_lock, NULL);
	pthread_cond_init(&tc->producer_cv, NULL);
	pthread_cond_init(&tc->consumer_cv, NULL);
	tc->mailbox.type = EV_NONE;

	/* Spawn a thread to run the test case */
	return pthread_create(&tc->thread, NULL,
                              (void *(*)(void *)) fn, tc);
}


void
test_destroy(struct test *tc) {
	pthread_mutex_destroy(&tc->mailbox_lock);
	pthread_cond_destroy(&tc->producer_cv);
	pthread_cond_destroy(&tc->consumer_cv);
	libxl_ctx_free(tc->ctx);
}


void
send_event(struct test *tc, struct event ev)
{
	pthread_mutex_lock(&tc->mailbox_lock);
	while (tc->mailbox.type != EV_NONE) {
		pthread_cond_wait(&tc->producer_cv, &tc->mailbox_lock);
	}
	tc->mailbox = ev;
	pthread_cond_signal(&tc->consumer_cv);
	pthread_mutex_unlock(&tc->mailbox_lock);
}


void
recv_event(struct test *tc, struct event *ev)
{
	pthread_mutex_lock(&tc->mailbox_lock);
	while (tc->mailbox.type == EV_NONE) {
		pthread_cond_wait(&tc->consumer_cv, &tc->mailbox_lock);
	}
	*ev = tc->mailbox;
	tc->mailbox.type = EV_NONE;
	pthread_cond_signal(&tc->producer_cv);
	pthread_mutex_unlock(&tc->mailbox_lock);
}


/* Wait until the an event matching the expected mask is posted.
   Ignores and discards any non-matching events received in the meantime. */
void
wait_for(struct test *tc, enum event_type expected)
{
	struct event received;
	do {
		recv_event(tc, &received);
	} while(!(received.type & expected));
}


/* Wait until the the specified number of the expected events are posted.
   Ignores and discards any other events received in the meantime. */
void
wait_for_n(struct test *tc, enum event_type expected, int count)
{
	while(count--) {
		wait_for(tc, expected);
	}
}


void
send_fd_event(struct test *tc, int fd)
{
	struct event ev;
	ev.type = EV_FD_EVENT;
	ev.u.fd = fd;
	send_event(tc, ev);
	printf("-> EV_FD_EVENT\n");
}


void
send_libxl_callback_event(struct test *tc)
{
	struct event ev;
	ev.type = EV_LIBXL_CALLBACK;
	send_event(tc, ev);
	printf("-> EV_LIBXL_CALLBACK\n");
}


void
send_eventloop(struct test *tc)
{
	struct event ev;
	ev.type = EV_EVENTLOOP;
	send_event(tc, ev);
	printf("-> EV_EVENTLOOP_TICK\n");
}

