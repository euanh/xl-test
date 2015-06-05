SRCS = testcase_runner.c eventloop_runner.c testcase_utils.c async_test.c
CFLAGS = -Wall -Wextra -Werror -pedantic -g
LDFLAGS = -Wl,-export-dynamic
LDLIBS = -pthread -lxenctrl -lxlutil -lxenlight -ldl
TESTS = $(wildcard test_*.c)
ALL_SRCS = $(SRCS) $(TESTS)

.PHONY: all
all: async_test $(TESTS:.c=.so)
async_test: $(SRCS:.c=.o)


test_%.o: CFLAGS += -fPIC

test_%.so: test_%.o
	$(CC) -shared $< -o $@

%.d: %.c
	$(CC) -M $< > $@

.PHONY: clean
clean:
	rm -f $(ALL_SRCS:.c=.o) $(ALL_SRCS:.c=.d) $(TESTS:.c=.so) async_test

-include $(ALL_SRCS:.c=.d)
