#include "../drivers/screen.h"

void main() {
    clear_screen();
    print_string("Hello World from C Kernel!\n");
    print_string("We are in Protected Mode.");
}
