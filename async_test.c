#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "testcase_runner.h"
#include "eventloop_runner.h"

void *testcase(struct test *tc);

int main(int argc __attribute__((unused)), 
         char **argv __attribute__((unused)))
{
    struct test t;
    xentoollog_logger *logger;

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

    exit(EXIT_SUCCESS);
}
