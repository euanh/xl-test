#include <stdlib.h>
#include <string.h>

#include "testcase_utils.h"
#include "testcase_runner.h"

void print_domain_config(libxl_ctx * ctx, char *msg, libxl_domain_config * dc)
{
    char *json = libxl_domain_config_to_json(ctx, dc);
    printf("%s: %s\n", msg, json);
    free(json);
}

void
_init_domain_config(libxl_domain_config * dc,
                   char *name, char *kernel, char *ramdisk,
                   char *hdd, char *cdrom)
{
    libxl_device_disk *disk;

    libxl_domain_config_init(dc);

    dc->c_info.name = strdup(name);
    dc->c_info.type = LIBXL_DOMAIN_TYPE_PV;
    dc->b_info.type = LIBXL_DOMAIN_TYPE_PV;
    dc->b_info.max_memkb = 512 * 1024;
    dc->b_info.u.pv.kernel = strdup(kernel);
    dc->b_info.u.pv.ramdisk = strdup(ramdisk);
    dc->b_info.cmdline = strdup("root=/dev/xvda1 selinux=0 console=hvc0");

    /* need to add devices here; create returns immediatly otherwise
       use xlu_disk_parse */
    dc->num_disks = 0;
    dc->disks = NULL;

    dc->disks = malloc(sizeof(*dc->disks) * 2);
    dc->num_disks = 2;

    disk = &dc->disks[0];
    libxl_device_disk_init(disk);
    disk->pdev_path = strdup(hdd);
    disk->vdev = strdup("xvda");
    disk->format = LIBXL_DISK_FORMAT_QCOW2;
    disk->readwrite = 1;

    disk = &dc->disks[1];
    libxl_device_disk_init(disk);
    disk->pdev_path = strdup(cdrom);
    disk->vdev = strdup("xvdb");
    disk->format = LIBXL_DISK_FORMAT_RAW;
    disk->removable = 1;
    disk->is_cdrom = 1;
}

void
init_domain_config(libxl_domain_config *dc, char *name)
{
    _init_domain_config(dc, name,
                        "resources/vmlinuz-4.0.4-301.fc22.x86_64",
                        "resources/initrd.xen-4.0.4-301.fc22.x86_64",
                        "resources/Fedora-Cloud-Base-22-20150521.x86_64.qcow2",
                        "resources/cloudinit.iso");
}

void generic_callback(libxl_ctx * ctx
                      __attribute__ ((unused)), int rc, void *for_callback)
{
    struct test *tc = for_callback;
    send_libxl_callback_event(tc, rc);
}

int
do_domain_create(struct test *t, libxl_domain_config * dc,
                 uint32_t * domid_out)
{
    t->ao_how.callback = generic_callback;
    t->ao_how.u.for_callback = t;

    return libxl_domain_create_new(t->ctx, dc, domid_out, &t->ao_how, 0);
}

int do_domain_suspend(struct test *t, uint32_t domid, int fd)
{
    t->ao_how.callback = generic_callback;
    t->ao_how.u.for_callback = t;

    return libxl_domain_suspend(t->ctx, domid, fd, 0, &t->ao_how);
}

int
do_domain_create_restore(struct test *t, libxl_domain_config * dc,
                         uint32_t * domid_out, int restore_fd,
                         libxl_domain_restore_params *params)
{
    t->ao_how.callback = generic_callback;
    t->ao_how.u.for_callback = t;

    return libxl_domain_create_restore(t->ctx, dc, domid_out, restore_fd, params, &t->ao_how, 0);
}

int do_domain_destroy(struct test *t, uint32_t domid)
{
    t->ao_how.callback = generic_callback;
    t->ao_how.u.for_callback = t;

    return libxl_domain_destroy(t->ctx, domid, &t->ao_how);
}

