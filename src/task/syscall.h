#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_PRINT 0

#include "../cpu/isr.h"

void syscall_dispatcher(registers_t *r);

#endif
