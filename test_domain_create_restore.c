#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "testcase_runner.h"
#include "eventloop_runner.h"
#include "testcase_utils.h"

/*
 * This test calls libxl_domain_create_restore() repeatedly, cancelling
 * it at different points in its lifecycle.   
 * 
 * Preconditions:
 *   No existing domain is called test_domain_create_restore.
 * 
 * Postconditions:
 *   After cancellation, the domain may or may not exist, and may be running.
 */

struct test_state {
    FILE *suspend_file;
    libxl_domain_config dc;
    libxl_domain_restore_params drp;
    uint32_t domid;
};

void setup_suite(struct test *tc, void **_state)
{
    struct event ev;
    libxl_domain_config dc;
    uint32_t domid = -2;

    struct test_state *st = malloc(sizeof *st);
    *_state = st;

    init_domain_config(&dc, "test_domain_create_restore");

    do_domain_create(tc, &dc, &domid);
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);
    assert(ev.u.callback_event.rc == 0);
    printf("domid: %d\n", domid);

    libxl_domain_unpause(tc->ctx, domid);
    printf("domain %d unpaused\n", domid);

    printf("waiting for domain to boot\n");
    wait_for_n(tc, EV_EVENTLOOP, 10, &ev);

    st->suspend_file = tmpfile();
    if (!(st->suspend_file)) {
        perror("tmpfile");
        exit(EXIT_FAILURE);
    }

    do_domain_suspend(tc, domid, fileno(st->suspend_file));
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);

    libxl_domain_destroy(tc->ctx, domid, 0);
    libxl_domain_config_dispose(&dc);
}


void 
setup(struct test *tc __attribute__((__unused__)), struct test_state *st)
{
    lseek(fileno(st->suspend_file), 0, SEEK_SET);
    init_domain_config(&st->dc, "test_domain_create_restore");
    libxl_domain_restore_params_init(&st->drp);
}


void
verify_cancelled(struct test *tc __attribute__((__unused__)), uint32_t domid, struct event ev)
{
    printf("libxl_domain_restore returned %d\n",
           ev.u.callback_event.rc);
    assert(ev.u.callback_event.rc == ERROR_CANCELLED
           || ev.u.callback_event.rc == 0);
    assert(domid != (uint32_t) -2);

    /* Restore was cancelled - the domain should not be running */
    /* assert(!libxl_domain_info(tc->ctx, NULL, domid)); */
}


void
verify_completed(struct test *tc, uint32_t domid, struct event ev)
{
    int rc;

    printf("libxl_domain_restore returned %d\n",
           ev.u.callback_event.rc);
    assert(ev.u.callback_event.rc == 0);
    libxl_domain_unpause(tc->ctx, domid);

    /* No operation in progress - cancelling should return an error */
    rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);
    printf("libxl_ao_cancel (after AO completion) returned %d\n", rc);
    assert(rc == ERROR_NOTFOUND);
}

bool
execute(struct test *tc, struct test_state *st, int count)
{
    struct event ev;
    int rc;

    printf("\n****** Will cancel after %d events ******\n", count);
    do_domain_create_restore(tc, &st->dc, &st->domid, fileno(st->suspend_file), &st->drp);

    if (wait_until_n(tc, EV_LIBXL_CALLBACK, count, &ev, 50)) {
        /* The API call returned before we could cancel it.
           It should have returned successfully.
         */
        verify_completed(tc, st->domid, ev);
        return false;
    }

    /*
     * Calling cancel on a cancellable operation should not return an
     * error, unless the operation happened to complete in the meantime.
     */
    rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);
    printf("libxl_ao_cancel returned %d\n", rc);
    assert(rc == ERROR_NOTFOUND || rc == 0);
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);

    verify_cancelled(tc, st->domid, ev);
    return true;
}

void
teardown(struct test *tc, struct test_state *st)
{
    assert(st->domid);
    libxl_domain_destroy(tc->ctx, st->domid, 0);
    libxl_domain_config_dispose(&st->dc);
}


void
teardown_suite(struct test *tc __attribute__((__unused__)),
               struct test_state *st)
{
    fclose(st->suspend_file);
    free(st);
}


void *testcase(struct test *tc)
{
    int count;
    void *state;
    setup_suite(tc, &state);

    for (count = 1; count < 100; count++) {
        bool cont;

        setup(tc, state);
        cont = execute(tc, state, count);
        teardown(tc, state);

        if (!cont) {
            break;
        }
    }

    teardown_suite(tc, state);
    test_exit();
    return NULL;
}
