#ifndef ISR_H
#define ISR_H

#include "types.h"

// Registers struct pushed by isr_common_stub
typedef struct {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code (pushed by our assembly stub)
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically
} registers_t;

void isr_install();
void isr_handler(registers_t regs);

#endif
