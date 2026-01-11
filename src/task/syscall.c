#include "syscall.h"
#include "../drivers/screen.h"
#include "../drivers/serial.h"
#include "../fs/file.h"

void syscall_dispatcher(registers_t *r) {
    switch (r->eax) {
        case SYSCALL_PRINT: {
            char *str = (char *)r->ebx;
            print_string(str);
            break;
        }
        case SYSCALL_OPEN: {
            char *filename = (char *)r->ebx;
            char *mode = (char *)r->ecx;
            r->eax = fopen(filename, mode);
            break;
        }
        case SYSCALL_READ: {
            void *ptr = (void *)r->ebx;
            uint32_t size = r->ecx;
            uint32_t nmemb = r->edx;
            int fd = (int)r->esi;
            r->eax = fread(ptr, size, nmemb, fd);
            break;
        }
        case SYSCALL_CLOSE: {
            int fd = (int)r->ebx;
            r->eax = fclose(fd);
            break;
        }
        case SYSCALL_STAT: {
            int fd = (int)r->ebx;
            struct file_stat *stat = (struct file_stat *)r->ecx;
            r->eax = fstat(fd, stat);
            break;
        }
        case SYSCALL_EXIT: {
            print_string("\nProcess execution finished. Halting.\n");
            __asm__("cli; hlt");
            break;
        }
    }
}
