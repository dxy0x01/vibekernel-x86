#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../cpu/idt.h"
#include "../drivers/serial.h"
#include "../drivers/ata.h"
#include "../fs/path_parser.h"

#include "../memory/heap/kheap.h"
#include "../memory/paging/paging.h"

#include "../cpu/gdt.h"
#include "../string/string.h"

#include "../drivers/disk_stream.h"
#include "../fs/fat16.h"
#include "../fs/file.h"
#include "panic.h"
#include "../task/task.h"
#include "../task/process.h"
#include "../drivers/keyboard.h"
#include "command.h"

void idt_init() {
    isr_install();
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void cls_handler(int argc, char** argv) {
    clear_screen();
}

void version_handler(int argc, char** argv) {
    print_string("VibeKernel-x86 v0.1.0\n");
}

void echo_handler(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        print_string(argv[i]);
        if (i < argc - 1) print_string(" ");
    }
    print_string("\n");
}

void main() {
    gdt_init();
    serial_init();
    clear_screen();
    print_string("VibeKernel-x86 Booting...\n");
    
    idt_init();
    kheap_init();
    fs_init();
    fs_insert_filesystem(fat16_init_vfs());
    
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));
    enable_paging();
    set_idt();
    __asm__ __volatile__("sti");

    keyboard_init();
    command_init();

    // Register basic commands
    extern void help_handler(int argc, char** argv);
    command_register("help", "Display this help message", help_handler);
    command_register("cls", "Clear the screen", cls_handler);
    command_register("version", "Display kernel version", version_handler);
    command_register("echo", "Print arguments to the screen", echo_handler);

    void print_handler(int argc, char** argv) {
        for (int i = 1; i < argc; i++) {
            print_string(argv[i]);
            if (i < argc - 1) print_string(" ");
        }
        print_string("\n");
    }
    command_register("print", "Display text on the screen", print_handler);

    char echo_msg[] = "echo VibeKernel is ready.";
    command_run(echo_msg);
    
    print_string("Type 'help' for a list of commands.\n");

    char cmd_buf[128];
    int cmd_idx = 0;

    while(1) {
        print_string("> ");
        cmd_idx = 0;
        memset(cmd_buf, 0, sizeof(cmd_buf));

        while (1) {
            char c = keyboard_getc();
            if (c == '\n') {
                print_string("\n");
                cmd_buf[cmd_idx] = '\0';
                break;
            } else if (c == '\b' || c == 0x7F) {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    print_string("\b \b");
                }
            } else {
                if (cmd_idx < sizeof(cmd_buf) - 1) {
                    cmd_buf[cmd_idx++] = c;
                    char s[2] = {c, 0};
                    print_string(s);
                }
            }
        }

        command_run(cmd_buf);
    }
}
