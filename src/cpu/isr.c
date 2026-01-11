#include "isr.h"
#include "idt.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "../drivers/serial.h"

// Declarations of external assembly symbols
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr128();

#include "../task/syscall.h"

// IRQs
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

void isr_install() {
    set_idt_gate(0, (uint32_t)isr0, 0);
    set_idt_gate(1, (uint32_t)isr1, 0);
    set_idt_gate(2, (uint32_t)isr2, 0);
    set_idt_gate(3, (uint32_t)isr3, 0);
    set_idt_gate(4, (uint32_t)isr4, 0);
    set_idt_gate(5, (uint32_t)isr5, 0);
    set_idt_gate(6, (uint32_t)isr6, 0);
    set_idt_gate(7, (uint32_t)isr7, 0);
    set_idt_gate(8, (uint32_t)isr8, 0);
    set_idt_gate(9, (uint32_t)isr9, 0);
    set_idt_gate(10, (uint32_t)isr10, 0);
    set_idt_gate(11, (uint32_t)isr11, 0);
    set_idt_gate(12, (uint32_t)isr12, 0);
    set_idt_gate(13, (uint32_t)isr13, 0);
    set_idt_gate(14, (uint32_t)isr14, 0);
    set_idt_gate(15, (uint32_t)isr15, 0);
    set_idt_gate(16, (uint32_t)isr16, 0);
    set_idt_gate(17, (uint32_t)isr17, 0);
    set_idt_gate(18, (uint32_t)isr18, 0);
    set_idt_gate(19, (uint32_t)isr19, 0);
    set_idt_gate(20, (uint32_t)isr20, 0);
    set_idt_gate(21, (uint32_t)isr21, 0);
    set_idt_gate(22, (uint32_t)isr22, 0);
    set_idt_gate(23, (uint32_t)isr23, 0);
    set_idt_gate(24, (uint32_t)isr24, 0);
    set_idt_gate(25, (uint32_t)isr25, 0);
    set_idt_gate(26, (uint32_t)isr26, 0);
    set_idt_gate(27, (uint32_t)isr27, 0);
    set_idt_gate(28, (uint32_t)isr28, 0);
    set_idt_gate(29, (uint32_t)isr29, 0);
    set_idt_gate(30, (uint32_t)isr30, 0);
    set_idt_gate(31, (uint32_t)isr31, 0);

    // Remap the PIC
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20); // Master offset 0x20
    port_byte_out(0xA1, 0x28); // Slave offset 0x28
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    port_byte_out(0x21, 0xFF); // Initially mask all
    port_byte_out(0xA1, 0xFF); 

    // Install IRQs
    set_idt_gate(32, (uint32_t)irq0, 0);
    set_idt_gate(33, (uint32_t)irq1, 0);
    set_idt_gate(34, (uint32_t)irq2, 0);
    set_idt_gate(35, (uint32_t)irq3, 0);
    set_idt_gate(36, (uint32_t)irq4, 0);
    set_idt_gate(37, (uint32_t)irq5, 0);
    set_idt_gate(38, (uint32_t)irq6, 0);
    set_idt_gate(39, (uint32_t)irq7, 0);
    set_idt_gate(40, (uint32_t)irq8, 0);
    set_idt_gate(41, (uint32_t)irq9, 0);
    set_idt_gate(42, (uint32_t)irq10, 0);
    set_idt_gate(43, (uint32_t)irq11, 0);
    set_idt_gate(44, (uint32_t)irq12, 0);
    set_idt_gate(45, (uint32_t)irq13, 0);
    set_idt_gate(46, (uint32_t)irq14, 0);
    set_idt_gate(47, (uint32_t)irq15, 0);

    // Syscall
    set_idt_gate(128, (uint32_t)isr128, 3); // DPL 3 for user access

    set_idt(); // Load IDT
}

/* To print the message which defines every exception */
char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t *r) {
    if (r->int_no < 32) {
        serial_print("received internal interrupt: ");
        serial_print(exception_messages[r->int_no]);
        serial_print("\n");
        
        if (r->int_no == 14) { // Page Fault
            uint32_t faulting_address;
            __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));
            serial_print("Faulting address: 0x");
            for (int i = 0; i < 8; i++) {
                uint8_t nibble = (faulting_address >> (28 - i * 4)) & 0xF;
                serial_putc(nibble < 10 ? nibble + '0' : nibble - 10 + 'A');
            }
            serial_print("\n");
        }

        print_string("received interrupt: ");
        print_string(exception_messages[r->int_no]);
        print_string("\n");
        __asm__("cli; hlt"); // Halt on exception
    }

    if (r->int_no == 128) {
        syscall_dispatcher(r);
    }
}

void irq_handler(registers_t *r) {
    // Send EOI to PICs
    if (r->int_no >= 40) port_byte_out(0xA0, 0x20); // Slave
    port_byte_out(0x20, 0x20); // Master

    // TOTALLY QUIET for stability
}
