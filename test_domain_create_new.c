#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include "testcase_runner.h"
#include "eventloop_runner.h"
#include "testcase_utils.h"

/*
 * This test calls libxl_domain_create_new() repeatedly, cancelling
 * it at different points in its lifecycle.   
 * 
 * Preconditions:
 *   No existing domain is called test_domain_create_new.
 * 
 * Postconditions:
 *   After cancellation, the partly created domain can be cleaned
 *   up by calling libxl_domain_destroy.
 *
 *   After cancellation and cleanup, another domain with the same
 *   name can be created.
 *
 * Improvements:
 *   Extracting the kernel and ramdisk from the root image, rather
 *   than supplying them directly, may provide another early 
 *   cancellation point.
 */

void *testcase(struct test *tc)
{
    int count;
    libxl_domain_config dc;

    init_domain_config(&dc, "test_domain_create_new",
                       "/root/vmlinuz-4.0.4-301.fc22.x86_64",
                       "/root/initrd.xen-4.0.4-301.fc22.x86_64",
                       "/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                       "/root/cloudinit.iso");

    for (count = 1; count < 100; count++) {
        uint32_t domid;
        struct event ev;
        int rc;

        printf("\n****** Will cancel after %d events ******\n", count);

        do_domain_create(tc, &dc, &domid);

        if (wait_until_n(tc, EV_LIBXL_CALLBACK, count, &ev)) {
            /* The API call returned before we could cancel it.
               It should have returned successfully.
             */
            printf("libxl_domain_create_new returned %d\n",
                   ev.u.callback_event.rc);
            assert(ev.u.callback_event.rc == 0);

            /* No operation in progress - cancelling should return an error */
            rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);
            printf("libxl_ao_cancel returned %d\n", rc);
            assert(rc == ERROR_NOTFOUND);

            libxl_domain_destroy(tc->ctx, domid, 0);
            break;
        }

        rc = libxl_ao_cancel(tc->ctx, &tc->ao_how);

        /* Calling cancel on a cancellable operation should not return an
           error, unless the operation happened to complete in the meantime.
         */
        printf("libxl_ao_cancel returned %d\n", rc);
        assert(rc == ERROR_NOTFOUND || rc == 0);

        /* The API call's return code should indicate that it was cancelled */
        wait_for(tc, EV_LIBXL_CALLBACK, &ev);
        printf("libxl_domain_create_new returned %d\n",
               ev.u.callback_event.rc);
        assert(ev.u.callback_event.rc == ERROR_CANCELLED
               || ev.u.callback_event.rc == 0);

        /* Although we cancelled, some domain configuration will have 
           been created. */
        printf("domid: %d\n", domid);
        assert(!libxl_domain_info(tc->ctx, NULL, domid));

        libxl_domain_destroy(tc->ctx, domid, 0);
    }

    libxl_domain_config_dispose(&dc);
    test_exit();
    return NULL;
}
