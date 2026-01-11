#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_PRINT 0
#define SYSCALL_OPEN  1
#define SYSCALL_READ  2
#define SYSCALL_CLOSE 3
#define SYSCALL_STAT  4
#define SYSCALL_EXIT  5

#include "../cpu/isr.h"

void syscall_dispatcher(registers_t *r);

#endif
