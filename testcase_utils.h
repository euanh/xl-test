#ifndef __TESTCASE_UTILS_H
#define __TESTCASE_UTILS_H

#include <libxl.h>

struct test;

void init_domain_config(libxl_domain_config * dc, char *name);

int do_domain_create(struct test *t, libxl_domain_config * dc,
                     uint32_t * domid_out);
int do_domain_create_restore(struct test *t, libxl_domain_config * dc,
                             uint32_t * domid_out, int restore_fd,
                             libxl_domain_restore_params *params);
int do_domain_suspend(struct test *t, uint32_t domid, int fd);
int do_domain_destroy(struct test *t, uint32_t domid);

#endif                          /* __TESTCASE_UTILS */
