#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include "testcase_runner.h"
#include "eventloop_runner.h"
#include "testcase_utils.h"

/*
 TODO:
   libxl__bootloader_run may add a cancellable point, but we don't enter
   it because we provide the kernel and ramdisk along with the root image.
 */


#if 0
void wait_or_exit(struct test *tc, libxl_domain_config *dc, int domid)
{
}
#endif


void teardown(struct test *tc, libxl_domain_config *dc, int domid)
{
    libxl_domain_config_dispose(dc);
    libxl_domain_destroy(tc->ctx, domid, 0);

}


void *testcase(struct test *tc)
{
    int count = 0;

    while (1) {
        uint32_t domid;
        libxl_domain_config dc;
        struct event ev;
        int i;

        count++;
        printf("\n****** Will cancel after %d events ******\n", count);

        init_domain_config(&dc, "test_domain_create_new",
                           "/root/vmlinuz-4.0.4-301.fc22.x86_64",
                           "/root/foobar.img",
                           "/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                           "/root/init.iso");

        do_domain_create(tc, &dc, &domid);

        /* Wait for some number of events before cancelling.
           Eventloop timeouts are ignored as they could happen at
           any time.  The test ends if the callback occurs while
           we are still waiting for an event - after the callback,
           the API call can no longer be cancelled.
         */
        for (i = 0; i < count; i++) {
            wait_for(tc, ~EV_EVENTLOOP, &ev);
            if (ev.type == EV_LIBXL_CALLBACK) {
		teardown(tc, &dc, domid);
		test_exit();
            }
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
