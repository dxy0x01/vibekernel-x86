#include "../drivers/screen.h"
#include "../cpu/isr.h"

void main() {
    clear_screen();
    isr_install();
    print_string("Hello World from C Kernel!\n");
    print_string("We are in Protected Mode.\n");
    __asm__ __volatile__("sti"); // Enable interrupts
}
