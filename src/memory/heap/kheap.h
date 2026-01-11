#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include "../../cpu/types.h"

void kheap_init();
void *kmalloc(uint32_t size);
void *kmalloc_a(uint32_t size);
void *kmalloc_p(uint32_t size, uint32_t *phys);
void *kmalloc_ap(uint32_t size, uint32_t *phys);
void kfree(void *ptr);

#endif
