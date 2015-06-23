SRCS = testcase_runner.c eventloop_runner.c testcase_utils.c async_test.c
CFLAGS = -Wall -Wextra -Werror -g
LDLIBS = -pthread -lxenctrl -lxlutil -lxenlight
TESTS = $(wildcard test_*.c)
ALL_SRCS = $(SRCS) $(TESTS)

.PHONY: all
all: $(TESTS:.c=)

test_%: $(SRCS:.c=.o) test_%.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.d: %.c
	$(CC) -M $< > $@

.PHONY: clean
clean:
	rm -f $(ALL_SRCS:.c=.o) $(ALL_SRCS:.c=.d) $(TESTS:.c=)

-include $(ALL_SRCS:.c=.d)
