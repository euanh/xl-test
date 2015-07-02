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

void setup_suite(struct test *tc, FILE **suspend_file)
{
    struct event ev;
    libxl_domain_config dc;
    uint32_t domid = -2;
    int suspend_fd;

    init_domain_config(&dc, "test_domain_create_restore",
                       "resources/vmlinuz-4.0.4-301.fc22.x86_64",
                       "resources/initrd.xen-4.0.4-301.fc22.x86_64",
                       "resources/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                       "resources/cloudinit.iso");

    do_domain_create(tc, &dc, &domid);
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);
    assert(ev.u.callback_event.rc == 0);
    printf("domid: %d\n", domid);

    libxl_domain_unpause(tc->ctx, domid);
    printf("domain %d unpaused\n", domid);

    printf("waiting for domain to boot\n");
    wait_for_n(tc, EV_EVENTLOOP, 10, &ev);	

    *suspend_file = tmpfile();
    if (!(*suspend_file)) {
        perror("tmpfile");
        exit(EXIT_FAILURE);
    }
    suspend_fd = fileno(*suspend_file);

    do_domain_suspend(tc, domid, suspend_fd);
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);

    libxl_domain_destroy(tc->ctx, domid, 0);
    libxl_domain_config_dispose(&dc);
}


void 
setup(struct test *tc __attribute__((__unused__)), libxl_domain_config *dc, FILE *suspend_file, libxl_domain_restore_params *params)
{
    lseek(fileno(suspend_file), 0, SEEK_SET);
    init_domain_config(dc, "test_domain_suspend",
                       "resources/vmlinuz-4.0.4-301.fc22.x86_64",
                       "resources/initrd.xen-4.0.4-301.fc22.x86_64",
                       "resources/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                       "resources/cloudinit.iso");
    libxl_domain_restore_params_init(params);
}


void
verify_cancelled(struct test *tc __attribute__((__unused__)), uint32_t domid, struct event ev)
{
    printf("libxl_domain_restore returned %d\n",
           ev.u.callback_event.rc);
    assert(ev.u.callback_event.rc == ERROR_FAIL  /* XXX should be ERROR_CANCELLED */
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


void
teardown(struct test *tc, uint32_t domid, libxl_domain_config *dc)
{
    assert(domid);
    libxl_domain_destroy(tc->ctx, domid, 0);
    libxl_domain_config_dispose(dc);
}


void
teardown_suite(FILE *suspend_file)
{
    fclose(suspend_file);
}


bool
execute(struct test *tc, uint32_t *domid, libxl_domain_config *dc, libxl_domain_restore_params *params, int count, FILE* suspend_file)
{
    struct event ev;
    int rc;

    printf("\n****** Will cancel after %d events ******\n", count);
    do_domain_create_restore(tc, dc, domid, fileno(suspend_file), params);

    if (wait_until_n(tc, EV_LIBXL_CALLBACK, count, &ev, 50)) {
        /* The API call returned before we could cancel it.
           It should have returned successfully.
         */
        verify_completed(tc, *domid, ev);
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

    verify_cancelled(tc, *domid, ev);
    return true;
}

void *testcase(struct test *tc)
{
    int count;
    FILE *suspend_file;
    setup_suite(tc, &suspend_file);

    for (count = 1; count < 100; count++) {
        libxl_domain_config dc;
        libxl_domain_restore_params params;
        uint32_t domid = -2;
        bool cont;

        setup(tc, &dc, suspend_file, &params);
        cont = execute(tc, &domid, &dc, &params, count, suspend_file);
        teardown(tc, domid, &dc);

        if (!cont) {
            break;
        }
    }

    teardown_suite(suspend_file);
    test_exit();
    return NULL;
}
