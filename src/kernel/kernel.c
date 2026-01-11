#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "mem.h"

void main() {
    clear_screen();
    isr_install();
    print_string("Hello World from C Kernel!\n");
    print_string("We are in Protected Mode.\n");
    __asm__ __volatile__("sti"); // Enable interrupts

    memory_init();
    
    // Test Malloc
    char* str = (char*)kmalloc(10);
    if (str) {
        str[0] = 'H'; str[1] = 'e'; str[2] = 'a'; str[3] = 'p'; str[4] = '!'; str[5] = 0;
        print_string("Allocated: ");
        print_string(str);
        print_string("\n");
        kfree(str);
        print_string("Freed.\n");
    } else {
        print_string("Malloc failed.\n");
    }
}
