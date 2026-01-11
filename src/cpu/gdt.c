#include "gdt.h"

extern void gdt_flush(uint32_t);

gdt_entry_t gdt[3];
gdt_ptr_t   gdt_ptr;

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    
    // Code segment: base=0, limit=0xffffffff, present, ring 0, code, exec/read, 4KB granularity, 32-bit
    gdt_set_gate(1, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_S | GDT_ACCESS_TYPE_CODE_EXREAD, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT); 

    // Data segment: base=0, limit=0xffffffff, present, ring 0, data, read/write, 4KB granularity, 32-bit
    gdt_set_gate(2, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_S | GDT_ACCESS_TYPE_DATA_RDWR, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);

    gdt_flush((uint32_t)&gdt_ptr);
}
