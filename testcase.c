#include <libxl.h>
#include "thread_test.h"
#include "xl_eventloop.h"

void *
testcase(struct test *tc)
{
	uint32_t domid;
	libxl_domain_config dc;

	printf("thread started\n");

	init_domain_config(&dc, "badger", "/root/vmlinuz-4.0.4-301.fc22.x86_64");
	do_domain_create(tc, &dc, &domid);
	wait_for(tc, EV_LIBXL_CALLBACK);  /* need to be able to distingish out callback from others */
	libxl_domain_config_dispose(&dc);
	printf("domain %d created\n", domid);

	libxl_domain_unpause(tc->ctx, domid);
	printf("domain %d unpaused\n", domid);

	wait_for_n(tc, EV_EVENTLOOP, 10);
	do_domain_suspend(tc, domid);
	wait_for(tc, EV_LIBXL_CALLBACK);
	printf("domain %d suspended\n", domid);

	pthread_exit(NULL);
}

