#include <assert.h>
#include <libxl.h>
#include <libxl_event.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/select.h>
#include <string.h>
#include "xl_eventloop.h"
#include "thread_test.h"

/*
 * These operations may need to be protected by a lock, since
 * the worker thread will make calls which require file descriptors
 * to be registered and the event loop will deregister them.
 * Alternatively, the test case will need to ask for file descriptors
 * to be registered and deregistered by sending messages to the main
 * event loop.
 */

void
init_pollfds(struct pollfd *pollfds, int numfds)
{
	int i;
	for (i = 0; i < numfds; i++) {
			pollfds[i].fd = -1;
			pollfds[i].events = 0;
			pollfds[i].revents = 0;
	}
}

int
add_poll_fd(struct pollfd *pollfds, int numfds, int fd, short events)
{
	int i;
	for (i = 0; i < numfds; i++) {
		if (pollfds[i].fd == -1) {
			pollfds[i].fd = fd;
			pollfds[i].events = events;
			pollfds[i].revents = 0;
			return i;
		}
	}

	return -1;
}

int
modify_poll_fd(struct pollfd *pollfds, int numfds, int slot, int fd, short events)
{
	assert(slot < numfds);
	assert(pollfds[slot].fd == fd);
	pollfds[slot].events = events;
	return 0;
}

int
remove_poll_fd(struct pollfd *pollfds, int numfds, int slot, int fd)
{
	assert(slot < numfds);
	assert (pollfds[slot].fd != -1);
	assert (pollfds[slot].fd == fd);
	pollfds[slot].fd = -1;
	pollfds[slot].events = 0;
	pollfds[slot].revents = 0;
	return 0;
}


int
fd_register(void *user, int fd, void **for_app_registration_out,
            short events, void *for_libxl)
{
	int slot;
	struct test *t;
	struct libxl_task *lxt;
	t = user;

	slot = add_poll_fd(pollfds, NUM_POLL_FDS, fd, events);
	lxt = &libxl_tasks[slot];
	lxt->task = t;
	lxt->slot = slot;
	lxt->for_libxl = for_libxl;
	*for_app_registration_out = lxt;

	return 0;
}

int
fd_modify(void *user, int fd, void **for_app_registration_update,
          short events)
{
	struct test *t;
	struct libxl_task *lxt;

	assert(user);
	assert(for_app_registration_update);
	assert(*for_app_registration_update);

	t = user;
	lxt = *for_app_registration_update;

	modify_poll_fd(pollfds, NUM_POLL_FDS, lxt->slot, fd, events);

	return 0;
}

void
fd_deregister(void *user, int fd, void *for_app_registration)
{
	struct test *t;
	struct libxl_task *lxt;


	t = user;
	lxt = for_app_registration;

	remove_poll_fd(pollfds, NUM_POLL_FDS, lxt->slot, fd);

}

int
timeout_register(void *user __attribute__((unused)),
                 void **for_app_registration_out __attribute__((unused)),
                 struct timeval abs __attribute__((unused)),
                 void *for_libxl __attribute__((unused)))
{
	return 0;
}

/* only ever called with abs={0,0}, meaning ASAP */
int
timeout_modify(void *user __attribute__((unused)), void **for_app_registration_update __attribute__((unused)),
               struct timeval abs __attribute__((unused)))
{
	return 0;
}

/* will never be called */
void
timeout_deregister(void *user __attribute__((unused)), void *for_app_registration __attribute__((unused)))
{
}


void
print_domain_config(libxl_ctx *ctx, char *msg, libxl_domain_config *dc) {
	char *json = libxl_domain_config_to_json(ctx, dc);
	printf("%s: %s\n", msg, json);
	free(json);
}


void
libxlEventHandler(void *data __attribute__((unused)), /* const */ libxl_event *event __attribute__((unused)))
{
}

void
register_callbacks(struct test *t)
{
	/*
         * Register the hook functions which libxl will call
         * to register its timers and its interest in file
         * descriptors used for operations such as suspend
         * and resume.
         * The structs containing these hooks must outlive
         * the xl context.
         * Each callback will be called with a pointer to the
         * task structure.
         */

	/* Register ordinary async callbacks */
	t->xl_ev_hooks.event_occurs_mask = LIBXL_EVENTMASK_ALL;
	t->xl_ev_hooks.event_occurs      = libxlEventHandler;
	t->xl_ev_hooks.disaster          = 0;
	libxl_event_register_callbacks(t->ctx, &t->xl_ev_hooks, t);

	/* Register eventloop integration callbacks */
	t->xl_os_ev_hooks.fd_register   = fd_register;
	t->xl_os_ev_hooks.fd_modify     = fd_modify;
	t->xl_os_ev_hooks.fd_deregister = fd_deregister;
	t->xl_os_ev_hooks.timeout_register   = timeout_register;
	t->xl_os_ev_hooks.timeout_modify     = timeout_modify;
	t->xl_os_ev_hooks.timeout_deregister = timeout_deregister;
	libxl_osevent_register_hooks(t->ctx, &t->xl_os_ev_hooks, t);
}

void domain_create_cb(libxl_ctx *ctx, int rc, void *for_callback);

void
init_domain_config(libxl_domain_config *dc, char *name, char *kernel)
{
        libxl_device_disk *disk;

	libxl_domain_config_init(dc);

	/* should we be using xlu_cfg_replace_string? */
	dc->c_info.name = strdup(name);
	dc->c_info.type = LIBXL_DOMAIN_TYPE_PV;
	dc->b_info.type = LIBXL_DOMAIN_TYPE_PV;
	dc->b_info.max_memkb = 512*1024;
	dc->b_info.u.pv.kernel = strdup(kernel);
	dc->b_info.u.pv.ramdisk = strdup("/root/foobar.img");
	dc->b_info.cmdline = strdup("root=/dev/xvda1 selinux=0 console=hvc0");

        /* need to add devices here; create returns immediatly otherwise
           use xlu_disk_parse? xlu_cfg_readfile? */
        dc->num_disks = 0;
        dc->disks = NULL;

        dc->disks = malloc(sizeof(*dc->disks) * 2);
        dc->num_disks = 2;

        disk = &dc->disks[0];
	libxl_device_disk_init(disk);
        disk->pdev_path = strdup("/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2");
        disk->vdev = strdup("xvda");
        disk->format = LIBXL_DISK_FORMAT_QCOW2;
	disk->readwrite = 1;

        disk = &dc->disks[1];
	libxl_device_disk_init(disk);
        disk->pdev_path = strdup("/root/init.iso");
        disk->vdev = strdup("xvdb");
        disk->format = LIBXL_DISK_FORMAT_RAW;
	disk->removable = 1;
	disk->is_cdrom = 1;
}

int
do_domain_create(struct test *t, libxl_domain_config *dc, uint32_t *domid_out)
{
	/* speak to andy cooper about valgrind runes to handle xen hypercalls */

	t->ao_how.callback = domain_create_cb;
	t->ao_how.u.for_callback = t; /* need to carry the other data to be freed */

	return libxl_domain_create_new(t->ctx, dc, domid_out, &t->ao_how, 0);
}

void
domain_create_cb(libxl_ctx *ctx __attribute__((unused)),
                 int rc __attribute__((unused)), void *for_callback)
{
        struct test *tc = for_callback;
	send_libxl_callback_event(tc);
}


void domain_suspend_cb(libxl_ctx *ctx, int rc, void *for_callback);

int
do_domain_suspend(struct test *t, uint32_t domid)
{
	/* need to issue a suspend in order to get an event channel wait
	 * which should ask to register an fd or something else for us above */
	int fd = open("/tmp/suspend", O_RDWR|O_CREAT|O_TRUNC);

	t->ao_how.callback = domain_suspend_cb;
	/* t->ao_how.u.for_callback = (void*) fd;  could rely on the test case to provide and close the fd - it can track it in a local variable */

	return libxl_domain_suspend(t->ctx, domid, fd, LIBXL_SUSPEND_LIVE, &t->ao_how);
}

void
domain_suspend_cb(libxl_ctx *ctx __attribute__((unused)),
                  int rc __attribute__((unused)),
                  void *for_callback __attribute__((unused)))
{
	/* struct test *t = for_callback; */
	/* close(t->in_args.domain_suspend.fd); */
	printf("< domain_suspend_cb()\n");
}

