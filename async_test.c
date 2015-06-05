#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "testcase_runner.h"
#include "eventloop_runner.h"

int main(int argc, char **argv)
{
    struct test t;
    xentoollog_logger *logger;
    char *test_path;
    void *test_plugin;
    void *(*testcase) (struct test *);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <testcase>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* dlopen only looks outside the standard library directories
       if the path contains a slash.   Resolve the real path to
       the test file, to ensure that this is the case. */
    test_path = realpath(argv[1], NULL);
    if (!test_path) {
        perror(argv[1]);
        exit(EXIT_FAILURE);
    }

    test_plugin = dlopen(test_path, RTLD_NOW);
    free(test_path);
    if (!test_plugin) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    dlerror();

    *(void **)(&testcase) = dlsym(test_plugin, "testcase");
    if (!testcase) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    dlerror();

    logger = (xentoollog_logger *)
        xtl_createlogger_stdiostream(stderr, XTL_PROGRESS, 0);

    if (! !test_spawn(&t, logger, testcase)) {
        perror("Failed to spawn test thread");
        exit(EXIT_FAILURE);
    }

    eventloop_start(&t);

    fprintf(stderr, "Waiting for test thread to exit\n");
    test_join(&t);
    fprintf(stderr, "Test thread exited\n");
    test_destroy(&t);
    xtl_logger_destroy(logger);
    dlclose(test_plugin);

    exit(EXIT_SUCCESS);
}
