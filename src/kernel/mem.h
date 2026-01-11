#ifndef MEM_H
#define MEM_H

#include "../cpu/types.h"

void memory_init();
void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
