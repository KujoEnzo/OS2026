#ifndef MODULE1_TEST_H
#define MODULE1_TEST_H
#include <trap.h>
void module1_run(void) __attribute__((noreturn));
void module1_prepare(unsigned long id);
void module1_handler(struct trapframe *tf);
void module1_post(void);
void module1_finish(void);
void module1_fail(const char *, uintptr_t, uintptr_t) __attribute__((noreturn));
#endif
