#ifndef __THREAD_TEST_H
#define __THREAD_TEST_H

#include <libxl.h>
#include <pthread.h>

enum event_type {
	EV_NONE 		= 0x0000,
	EV_TEST_START           = 0x0001,
	EV_LIBXL_CALLBACK	= 0x0002,
	EV_FD_EVENT		= 0x0004,
	EV_EVENTLOOP		= 0x0008,
	EV_ANY 			= 0xffff
};


struct event {
	enum event_type type;
	union {
		int fd;
	} u;
};

struct test {
	/*
         * Test task structure.
         */

	/*
         * Pointer to the libxl context structure.
         * The memory which this points to is allocated and
         * owned by libxl.
         */
	libxl_ctx *ctx;

	/*
         * Structures containing the hooks which libxl
         * uses to register its timers and its interest in
         * file descriptors.
         * These structures must outlive the libxl context
         * in which they are used, so it makes sense to
         * keep them here alongside the ctx pointer.
         */
	libxl_event_hooks xl_ev_hooks;
	libxl_osevent_hooks xl_os_ev_hooks;

	/*
	 * Structure defining per-function callbacks.
	 * Must outlive the asynchronous call which uses it.
         */
	libxl_asyncop_how ao_how;

	/* The thread running the test */
	pthread_t thread;

	/* Test thread's mailbox and locks */
	pthread_mutex_t mailbox_lock;
	pthread_cond_t producer_cv;
	pthread_cond_t consumer_cv;
	struct event mailbox;
};


int test_spawn(struct test *tc, xentoollog_logger *logger,
               void *(*fn)(struct test *));
void test_destroy(struct test *tc);
void send_event(struct test *tc, struct event ev);
void recv_event(struct test *tc, struct event *ev);
void wait_for(struct test *tc, enum event_type expected);
void wait_for_n(struct test *tc, enum event_type expected, int count);
void send_fd_event(struct test *tc, int fd);
void send_libxl_callback_event(struct test *tc);
void send_eventloop(struct test *tc);

#endif /* __THREAD_TEST_H */
