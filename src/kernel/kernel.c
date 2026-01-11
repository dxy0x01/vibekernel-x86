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

// Simulate idt_init if it doesn't match exactly yet
void idt_init() {
    isr_install();
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void kernel_tests() {
    // 1. Dynamic Mapping Test
    uint32_t vga_phys = 0xB8000;
    void* vga_virt = (void*)0xC0000000;
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), vga_virt, vga_phys | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);
    char* vga = (char*)vga_virt;
    vga[0] = 'X'; 
    vga[1] = 0x0F; 
    
    if (vga[0] == 'X') {
        serial_print("Paging: Verification SUCCESS!\n");
    }

    // 2. Alignment logic
    void* ptr1 = kmalloc_a(100);
    if (((uint32_t)ptr1 & 0xFFF) == 0) {
        serial_print("Alignment: Verification SUCCESS!\n");
    }

    // 3. Disk Stream
    uint16_t* write_buf = (uint16_t*)kmalloc(512);
    for(int i=0; i<256; i++) write_buf[i] = 0x55AA;
    ata_write_sector(0, 2, write_buf);
    kfree(write_buf);

    struct disk_stream* stream = diskstream_new(0);
    diskstream_seek(stream, 512 * 2); 
    uint16_t read_val = 0;
    if (diskstream_read(stream, &read_val, 2) == 0 && read_val == 0x55AA) {
        serial_print("Disk Stream: Verification SUCCESS!\n");
    }
    diskstream_close(stream);

    // 4. Path Parser
    struct path_root* root = path_parser_parse("0:/home/usr/test.txt", NULL);
    if (root) {
        serial_print("Path Parser: Verification SUCCESS!\n");
        path_parser_free(root);
    }

    // 5. FAT16/VFS (Inject and Read)
    struct disk drive1 = {1, NULL};
    if (fat16_resolve(&drive1) == 0) {
        // Prepare clusters
        uint32_t root_sec = fat16_get_root_directory_sector(&drive1); 
        uint16_t* root_buf = (uint16_t*)kmalloc(512);
        ata_read_sector(1, root_sec, root_buf);
        struct fat_directory_item* dir_entry = (struct fat_directory_item*)root_buf;
        memcpy(&dir_entry[10], "DIR     ", 8);
        dir_entry[10].attribute = 0x10; 
        dir_entry[10].low_16_bits_first_cluster = 3; 
        ata_write_sector(1, root_sec, root_buf);
        
        uint32_t dir_sec = fat16_cluster_to_sector(&drive1, 3);
        uint16_t* dir_buf = (uint16_t*)kmalloc(512);
        memset(dir_buf, 0, 512);
        struct fat_directory_item* file_entry = (struct fat_directory_item*)dir_buf;
        memcpy(file_entry->filename, "TEST    ", 8);
        memcpy(file_entry->ext, "TXT", 3);
        file_entry->attribute = 0x20;
        file_entry->low_16_bits_first_cluster = 4; 
        file_entry->filesize = 1011; 
        ata_write_sector(1, dir_sec, dir_buf);
        
        uint32_t file_sec = fat16_cluster_to_sector(&drive1, 4);
        char* large_buf = (char*)kmalloc(1024);
        memset(large_buf, 'A', 1024);
        memcpy(large_buf, "Start of large file", 19);
        memcpy(large_buf + 1000, "End of file", 11);
        ata_write_sector(1, file_sec, (uint16_t*)large_buf);
        ata_write_sector(1, file_sec + 1, (uint16_t*)(large_buf + 512));
        
        uint32_t fat_sec = ((struct fat_private*)drive1.fs_private)->bpb.reserved_sectors;
        uint16_t* fat_buf = (uint16_t*)kmalloc(512);
        ata_read_sector(1, fat_sec, fat_buf);
        fat_buf[4] = 5; fat_buf[5] = 0xFFFF;
        ata_write_sector(1, fat_sec, fat_buf);
        kfree(fat_buf);
        
        int fd = fopen("1:/DIR/TEST.TXT", "r");
        if (fd > 0) {
            char rb[32];
            fread(rb, 1, 19, fd);
            fseek(fd, 1000, FILE_SEEK_SET);
            fread(rb, 1, 11, fd);
            if (ftell(fd) == 1011) {
                serial_print("VFS: Verification SUCCESS!\n");
            }
            fclose(fd);
        }
    }
}

void user_test_app() {
    // This should trigger a GPF because hlt is privileged
    __asm__ __volatile__("hlt");
    while(1);
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
    
    void cls_handler(int argc, char** argv) {
        clear_screen();
    }
    command_register("cls", "Clear the screen", cls_handler);

    void version_handler(int argc, char** argv) {
        print_string("VibeKernel-x86 v0.1.0\n");
    }
    command_register("version", "Display kernel version", version_handler);

    void echo_handler(int argc, char** argv) {
        for (int i = 1; i < argc; i++) {
            print_string(argv[i]);
            if (i < argc - 1) print_string(" ");
        }
        print_string("\n");
    }
    command_register("echo", "Print arguments to the screen", echo_handler);

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
