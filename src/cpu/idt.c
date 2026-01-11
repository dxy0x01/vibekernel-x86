#include "idt.h"

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

void set_idt_gate(int n, uint32_t handler, uint8_t dpl) {
    idt[n].low_offset = low_16(handler);
    idt[n].sel = KERNEL_CS;
    idt[n].always0 = 0;
    // flags: P (1), DPL (dpl), 0 (Reserved), Type (1110 for 32-bit int gate)
    idt[n].flags = 0x8E | ((dpl & 3) << 5); 
    idt[n].high_offset = high_16(handler);
}

void set_idt() {
    idt_reg.base = (uint32_t) &idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    __asm__ __volatile__("lidt (%0)" : : "r" (&idt_reg));
}
