#ifndef __THREAD_TEST_H
#define __THREAD_TEST_H

#include <libxl.h>
#include <pthread.h>

enum event_type {
    EV_NONE = 0x0000,
    EV_TEST_START = 0x0001,
    EV_LIBXL_CALLBACK = 0x0002,
    EV_FD_EVENT = 0x0004,
    EV_EVENTLOOP = 0x0008,
    EV_LIBXL_EVENT = 0x0010,
    EV_ANY = 0xffff
};

struct event {
    enum event_type type;
    union {
        struct {
            int fd;
        } fd_event;
        struct {
            int rc;
        } callback_event;
        struct {
            libxl_event_type type;
        } libxl_event;
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
     * Structure defining per-function callbacks.
     * Must outlive the asynchronous call which uses it.
     */
    libxl_asyncop_how ao_how;

    /* The thread running the test */
    pthread_t thread;
    int mailbox[2];
};

int test_spawn(struct test *tc, xentoollog_logger * logger,
               void *(*fn) (struct test *));
void test_join(struct test *tc);
void test_exit();
void test_destroy(struct test *tc);
int send_event(struct test *tc, struct event ev);
void recv_event(struct test *tc, struct event *ev);
void wait_for(struct test *tc, enum event_type mask, struct event *ev);
void wait_for_n(struct test *tc, enum event_type mask, int count,
                struct event *ev);
int wait_until_n(struct test *tc, enum event_type mask, int count,
                 struct event *ev);
int send_fd_event(struct test *tc, int fd);
int send_libxl_callback_event(struct test *tc, int rc);
int send_libxl_event(struct test *tc, libxl_event_type type);
int send_eventloop_timeout(struct test *tc);

#endif                          /* __THREAD_TEST_H */
