#include "gdt.h"

extern void gdt_flush(uint32_t);
extern void tss_load(uint16_t);

gdt_entry_t gdt[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_gate(num, base, limit, GDT_ACCESS_TSS, 0x00);

    for (int i = 0; i < sizeof(tss_entry); i++) {
        ((char*)&tss_entry)[i] = 0;
    }

    tss_entry.ss0  = ss0;
    tss_entry.esp0 = esp0;

    // Set segments to kernel segments (with RPL 0)
    tss_entry.cs = 0x08;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x10;
}

void gdt_init() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
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

    // User Code segment: base=0, limit=0xffffffff, present, ring 3, code, exec/read, 4KB granularity, 32-bit
    gdt_set_gate(3, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_TYPE_CODE_EXREAD, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);

    // User Data segment: base=0, limit=0xffffffff, present, ring 3, data, read/write, 4KB granularity, 32-bit
    gdt_set_gate(4, 0, 0xFFFFFFFF, 
                 GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_TYPE_DATA_RDWR, 
                 GDT_GRAN_4KB | GDT_GRAN_32BIT);

    // Initialize TSS (Selector 0x28)
    write_tss(5, 0x10, 0x00); 

    gdt_flush((uint32_t)&gdt_ptr);
    tss_load(0x28);
}
