#include "syscall.h"
#include "../drivers/screen.h"
#include "../drivers/serial.h"

void syscall_dispatcher(registers_t *r) {
    if (r->eax == SYSCALL_PRINT) {
        char *str = (char *)r->ebx;
        print_string(str);
    }
}
