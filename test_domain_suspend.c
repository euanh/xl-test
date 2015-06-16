#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include "testcase_runner.h"
#include "eventloop_runner.h"
#include "testcase_utils.h"

void *testcase(struct test *tc)
{
    uint32_t domid;
    struct event ev;
    libxl_domain_config dc;

    printf("thread started\n");

    init_domain_config(&dc, "test_domain_suspend",
                       "/root/vmlinuz-4.0.4-301.fc22.x86_64",
                       "/root/initrd.xen-4.0.4-301.fc22.x86_64",
                       "/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                       "/root/init.iso");
    do_domain_create(tc, &dc, &domid);

    wait_for(tc, EV_LIBXL_CALLBACK, &ev);
    printf("domid: %d\n", domid);
    libxl_domain_config_dispose(&dc);

    libxl_domain_unpause(tc->ctx, domid);
    printf("domain %d unpaused\n", domid);

    printf("waiting for domain to boot\n");
    wait_for_n(tc, EV_EVENTLOOP, 10, &ev);

    /* Start a suspend, and immediately cancel it */
    do_domain_suspend(tc, domid);
    wait_for(tc, EV_FD_EVENT, &ev);
    libxl_ao_cancel(tc->ctx, &tc->ao_how);

    wait_for(tc, EV_LIBXL_CALLBACK, &ev);
    printf("domain %d suspended\n", domid);

    eventloop_halt();
    pthread_exit(NULL);
}
