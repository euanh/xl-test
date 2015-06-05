#ifndef __XL_EVENTLOOP_H
#define __XL_EVENTLOOP_H

struct test;

void eventloop_start(struct test *tc);
void eventloop_halt();

#endif                          /* __XL_EVENTLOOP_H */
