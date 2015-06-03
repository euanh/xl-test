#ifndef __XL_EVENTLOOP_H
#define __XL_EVENTLOOP_H

#include <libxl.h>
#include <libxl_event.h>

struct test;
struct libxl_task {
	struct test *task;
	int slot;
	void *for_libxl;
};


enum { NUM_POLL_FDS = 10 };
struct pollfd pollfds[NUM_POLL_FDS];
struct libxl_task libxl_tasks[NUM_POLL_FDS];

void init_pollfds(struct pollfd *pollfds, int numfds);

void init_domain_config(libxl_domain_config *dc, char *name, char *kernel);

void register_callbacks(struct test *t);
int do_domain_create(struct test *t, libxl_domain_config *dc, uint32_t *domid_out);
int do_domain_suspend(struct test *t, uint32_t domid);

#endif /* __XL_EVENTLOOP_H */
