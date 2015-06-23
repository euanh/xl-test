#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include <stdlib.h>
#include <unistd.h>
#include "testcase_runner.h"
#include "eventloop_runner.h"
#include "testcase_utils.h"

/*
 * This test calls libxl_domain_destroy() repeatedly, cancelling
 * it at different points in its lifecycle.   
 * 
 * Preconditions:
 *   No existing domain is called test_domain_destroy.
 * 
 * Postconditions:
 *   After cancellation, either:
 *     - the domain exists and is running, or
 *     - the domain has been destroyed.
 *
 *   If the domain is still running after cancellation, it can be
 *   destroyed (and cancelled) again.
 * 
 * Problems:
 *   If we cancel after a certain point, the domain will have been
 *   destroyed and won't respond to another request to destroy it.
 *   The test logic must check whether the domain is running before
 *   trying to destroy.
 */

void *testcase(struct test *tc)
{
    int count;

    libxl_domain_config dc;
    uint32_t domid = -2;
    struct event ev;

    init_domain_config(&dc, "test_domain_destroy",
                       "resources/vmlinuz-4.0.4-301.fc22.x86_64",
                       "resources/initrd.xen-4.0.4-301.fc22.x86_64",
                       "resources/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                       "resources/cloudinit.iso");

    do_domain_create(tc, &dc, &domid);
    wait_for(tc, EV_LIBXL_CALLBACK, &ev);
    assert(ev.u.callback_event.rc == 0);
    assert(domid != (uint32_t) -2);
    printf("domid: %d\n", domid);

    libxl_domain_unpause(tc->ctx, domid);
    printf("domain %d unpaused\n", domid);

    printf("waiting for domain to boot\n");
    wait_for_n(tc, EV_EVENTLOOP, 10, &ev);

    for (count = 1; count < 100; count++) {
        int rc;

        printf("\n****** Will cancel after %d events ******\n", count);
        rc = do_domain_destroy(tc, domid);
        assert(rc == 0);

        if (wait_until_n(tc, EV_LIBXL_CALLBACK, count, &ev, 0)) {
            /* The API call returned before we could cancel it.
               It should have returned successfully.
             */
            printf("libxl_domain_destroy returned %d\n",
                   ev.u.callback_event.rc);
            assert(ev.u.callback_event.rc == 0);

            /* No operation in progress - cancelling should return an error */
            rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);
            printf("libxl_ao_cancel returned %d\n", rc);
            assert(rc == ERROR_NOTFOUND);

            break;
        }

        /* The wait_until_n() call did not receive a callback event,
           so we will try to cancel the asynchronous operation */

        printf("Cancelling asynchronous operation\n");
        rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);

        /* Calling cancel on a cancellable operation should not return an
           error, unless the operation happened to complete in the meantime.
         */
        printf("libxl_ao_cancel returned %d\n", rc);
        assert(rc == ERROR_NOTFOUND || rc == 0);

        /* The API call's return code should indicate that it was cancelled */
        wait_for(tc, EV_LIBXL_CALLBACK, &ev);
        printf("libxl_domain_destroy returned %d\n", ev.u.callback_event.rc);
        assert(ev.u.callback_event.rc == ERROR_CANCELLED
               || ev.u.callback_event.rc == 0);

        /* Destroy was cancelled - the domain should still be running */
        assert(!libxl_domain_info(tc->ctx, NULL, domid));
    }

    libxl_domain_destroy(tc->ctx, domid, 0);
    libxl_domain_config_dispose(&dc);
    test_exit();
    return NULL;
}
