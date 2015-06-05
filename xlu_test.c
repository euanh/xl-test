/* 
gcc -Wall -Werror -Wextra -pedantic  -g -lxenlight -lxlutil xlu_test.c -o xlu_test
*/

#include <assert.h>
#include <libxl.h>
#include <libxlutil.h>
#include <libxl_event.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <sys/select.h>
#include <string.h>

void disk_test(XLU_Config * config, const char *spec)
{
    libxl_device_disk disk;
    libxl_device_disk_init(&disk);
    xlu_disk_parse(config, 1, &spec, &disk);
}

int main(int argc, char **argv)
{
    int err;
    XLU_Config *config;
    XLU_ConfigList *config_list;
    int count;

    FILE *report;
    char *report_filename = "/tmp/xlu.out";
    const char *response;

    if (argc != 2) {
        fprintf(stderr, "%s: no config file provided\n", argv[0]);
        exit(-1);
    }

    report = fopen(report_filename, "w+");

    config = xlu_cfg_init(report, report_filename);
    if (!config) {
        perror("xlu_cfg_init");
        exit(-1);
    }

    err = xlu_cfg_readfile(config, argv[1]);
    if (err != 0) {
        perror("xlu_cfg_readfile");
        exit(-1);
    }

    err = xlu_cfg_get_string(config, "name", &response, 1);
    if (err != 0) {
        perror("xlu_cfg_get_string");
        exit(-1);
    }
    printf("name: %s\n", response);

    err = xlu_cfg_get_string(config, "kernel", &response, 1);
    if (err != 0) {
        perror("xlu_cfg_get_string");
        exit(-1);
    }
    printf("kernel: %s\n", response);

    err = xlu_cfg_get_list(config, "disk", &config_list, &count, 1);
    if (err != 0) {
        perror("xlu_cfg_get_string");
        exit(-1);
    }
    printf("disk: %d items\n", count);

    disk_test(config,
              "qcow2:/root/Fedora-Cloud-Base-22-20150521.x86_64.qcow2,xvda,w");
    disk_test(config, "file:/root/init.iso,xvdb:cdrom,r");
    xlu_cfg_destroy(config);
    exit(0);
}
