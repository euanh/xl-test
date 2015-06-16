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


void teardown(struct test *tc, libxl_domain_config * dc, int domid)
{
    libxl_domain_config_dispose(dc);
    libxl_domain_destroy(tc->ctx, domid, 0);

}

void *testcase(struct test *tc)
{
    int count;

    for (count = 1; count < 100; count++) {
        uint32_t domid;
        libxl_domain_config dc;
        struct event ev;

        printf("\n****** Will cancel after %d events ******\n", count);

        init_domain_config(&dc, "test_domain_create_new",
                           "/root/vmlinuz-4.0.4-301.fc22.x86_64",
                           "/root/foobar.img",
                           "/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                           "/root/init.iso");

        do_domain_create(tc, &dc, &domid);

        if (wait_until_n(tc, EV_LIBXL_CALLBACK, count, &ev)) {
            teardown(tc, &dc, domid);
            break;
        }

        libxl_ao_cancel(tc->ctx, &tc->ao_how);
        wait_for(tc, EV_LIBXL_CALLBACK, &ev);

        printf("domid: %d\n", domid);
        assert(!libxl_domain_info(tc->ctx, NULL, domid));

        teardown(tc, &dc, domid);
    }

    test_exit();
    return NULL;
}
