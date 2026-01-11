#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_entry_struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

struct gdt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

#define GDT_ACCESS_PRESENT     0x80
#define GDT_ACCESS_DPL0        0x00
#define GDT_ACCESS_DPL3        0x60
#define GDT_ACCESS_S           0x10
#define GDT_ACCESS_TYPE_CODE_EXREAD 0x0A
#define GDT_ACCESS_TYPE_DATA_RDWR   0x02

#define GDT_GRAN_4KB           0x80
#define GDT_GRAN_32BIT         0x40

void gdt_init();

#endif
