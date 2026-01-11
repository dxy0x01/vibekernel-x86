#include "../drivers/screen.h"
#include "../cpu/isr.h"

void main() {
    clear_screen();
    isr_install();
    print_string("Hello World from C Kernel!\n");
    print_string("We are in Protected Mode.\n");
    
    // Trigger interrupt
    // __asm__ __volatile__("int $2");
    // __asm__ __volatile__("int $3");
}
