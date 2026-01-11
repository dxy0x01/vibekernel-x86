#include "panic.h"
#include "../drivers/screen.h"
#include "../drivers/serial.h"

void panic(const char* msg) {
    print_set_colour(0x4F); // Red background, white text
    print("\nKERNEL PANIC: ");
    print(msg);
    serial_print("\nKERNEL PANIC: ");
    serial_print(msg);
    while(1) {
        asm volatile("cli; hlt");
    }
}
