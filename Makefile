SRCS = thread_test.c xl_eventloop.c testcase.c xl_event_test.c
CFLAGS = -Wall -Wextra -Werror -pedantic -g
LDFLAGS = -pthread -lxenlight

all: xl_event_test
xl_event_test: $(SRCS:.c=.o)

%.d: %.c
	$(CC) -M $< > $@

.PHONY: clean
clean:
	rm -f $(SRCS:.c=.o)  $(SRCS:.c=.d) xl_event_test

-include $(SRCS:.c=.d)
