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

// Simulate idt_init if it doesn't match exactly yet
void idt_init() {
    isr_install();
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void main() {
    gdt_init();
    serial_init();
    serial_print("GDT and Serial Initialized.\n");

    clear_screen();
    print_string("Hello World from C Kernel!\n");
    serial_print("Hello World from C Kernel!\n");
    
    idt_init();
    kheap_init();
    serial_print("Heap Initialized.\n");
    
    // Setup paging
    // flags: RW | Present | Access from all (User)
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    serial_print("Paging chunk created.\n");
    
    // Switch to kernel paging chunk
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));
    serial_print("Paging directory switched.\n");

    // Enable paging
    enable_paging();
    serial_print("Paging Enabled.\n");

    // Reload IDT at virtual address (identity mapped)
    set_idt();
    serial_print("IDT Reloaded.\n");

    // Mask all IRQs except 0 (Timer) and 1 (Keyboard)
    // 0xFC = 1111 1100
    port_byte_out(0x21, 0xFC);
    port_byte_out(0xA1, 0xFF);

    __asm__ __volatile__("sti");

    print_string("Paging Initialized. Interrupts ENABLED.\n");
    
    // Test Dynamic Mapping
    // Map Physical 0xB8000 (VGA) to Virtual 0xC0000000
    // Flags: Present | RW | User (3 or 7)
    uint32_t vga_phys = 0xB8000;
    void* vga_virt = (void*)0xC0000000;
    serial_print("Attempting to map 0xB8000 to 0xC0000000...\n");
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), vga_virt, vga_phys | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);
    serial_print("Mapping set.\n");

    char* vga = (char*)vga_virt;
    serial_print("Writing 'X' to 0xC0000000...\n");
    vga[0] = 'X'; // Should appear at top-left
    vga[1] = 0x0F; // White on black
    
    if (vga[0] == 'X') {
        print_string("Verification: 'X' read back from 0xC0000000 successfully!\n");
        serial_print("Verification: 'X' read back from 0xC0000000 successfully!\n");
    } else {
        print_string("Verification: FAILED to read 'X' from 0xC0000000.\n");
        serial_print("Verification: FAILED to read 'X' from 0xC0000000.\n");
    }
    
    print_string("Dynamic Mapping Test Done. Check screen top-left for 'X'.\n");
    serial_print("Dynamic Mapping Test Done.\n");

    // Test Alignment logic
    void* ptr1 = kmalloc_a(100);
    print_string("Aligned Malloc: 0x");
    
    // Check bits 0-11 are zero
    if (((uint32_t)ptr1 & 0xFFF) == 0) {
        print_string("Alignment Success!\n");
        serial_print("Alignment Success!\n");
    } else {
        print_string("Alignment FAILED!\n");
        serial_print("Alignment FAILED!\n");
    }

    // Test Disk Stream
    serial_print("Testing Disk Stream Layer...\n");
    
    // Write pattern to Sector 2 using raw ATA first
    uint16_t* write_buf = (uint16_t*)kmalloc(512);
    for(int i=0; i<256; i++) write_buf[i] = 0x55AA; // Swap pattern for variety
    ata_write_sector(0, 2, write_buf);

    struct disk_stream* stream = diskstream_new(0);
    diskstream_seek(stream, 512 * 2); // Seek to Sector 2

    uint16_t read_val = 0;
    if (diskstream_read(stream, &read_val, 2) == 0) {
        if (read_val == 0x55AA) {
            serial_print("Disk Stream Verification Success: Read back 0x55AA from offset 1024!\n");
        } else {
            serial_print("Disk Stream Verification FAILED: Read wrong value!\n");
        }
    } else {
        serial_print("Disk Stream Read FAILED!\n");
    }

    // Test unaligned read across sector boundary
    // Offset 510 (last 2 bytes of sector 1) to offset 514 (first 2 bytes of sector 2)
    diskstream_seek(stream, 512 * 2 - 2); 
    uint32_t cross_val = 0;
    if (diskstream_read(stream, &cross_val, 4) == 0) {
        serial_print("Cross-sector read successful.\n");
    }

    diskstream_close(stream);

    // Test Path Parser
    serial_print("Testing Path Parser...\n");
    struct path_root* root = path_parser_parse("0:/home/usr/test.txt", NULL);
    if (root) {
        serial_print("Parsed Drive: ");
        serial_putc(root->drive_no + '0');
        serial_print("\nParts:\n");
        struct path_part* part = root->first;
        while (part) {
            serial_print(" - ");
            serial_print((char*)part->part);
            serial_print("\n");
            part = part->next;
        }
        serial_print("Freeing path parser root...\n");
        path_parser_free(root);
        serial_print("Freeing Success.\n");
    } else {
        serial_print("Path parsing FAILED!\n");
    }

    // Test FAT16
    serial_print("Testing FAT16 Filesystem on Drive 1...\n");
    
    // --- Recursive VFS Verification: Manual Injection ---
    serial_print("Injecting DIR/TEST.TXT into Drive 1...\n");
    
    struct disk drive1 = {1, NULL};
    int fat_res = fat16_resolve(&drive1); // Set up drive1.fs_private
    if (fat_res == 0) {
        serial_print("FAT16 Initialized Successfully!\n");
        uint32_t root_sec = fat16_get_root_directory_sector(&drive1); 
        
        uint16_t* root_buf = (uint16_t*)kmalloc(512);
        ata_read_sector(1, root_sec, root_buf);
        
        struct fat_directory_item* dir_entry = (struct fat_directory_item*)root_buf;
        memcpy(dir_entry->filename, "DIR     ", 8);
        memcpy(dir_entry->ext, "   ", 3);
        dir_entry->attribute = 0x10; // Subdirectory
        dir_entry->low_16_bits_first_cluster = 3; // DIR data starts at cluster 3
        
        ata_write_sector(1, root_sec, root_buf);
        
        // Prepare DIR cluster (containing TEST.TXT)
        uint32_t dir_sec = fat16_cluster_to_sector(&drive1, 3);
        uint16_t* dir_buf = (uint16_t*)kmalloc(512);
        memset(dir_buf, 0, 512);
        struct fat_directory_item* file_entry = (struct fat_directory_item*)dir_buf;
        memcpy(file_entry->filename, "TEST    ", 8);
        memcpy(file_entry->ext, "TXT", 3);
        file_entry->attribute = 0x20;
        file_entry->low_16_bits_first_cluster = 4; // FILE data starts at cluster 4
        file_entry->filesize = 18; 
        
        // Prepare FILE data (across sector boundary if possible, but for now just larger)
        uint32_t file_sec = fat16_cluster_to_sector(&drive1, 4);
        char* large_buf = (char*)kmalloc(1024);
        memset(large_buf, 'A', 1024); // Fill with 'A'
        memcpy(large_buf, "Start of large file", 19);
        memcpy(large_buf + 1000, "End of file", 11);
        
        // Write two sectors for cluster 4 (assuming 512 byte sectors)
        ata_write_sector(1, file_sec, (uint16_t*)large_buf);
        ata_write_sector(1, file_sec + 1, (uint16_t*)(large_buf + 512));
        
        // Update FAT table to link cluster 4 -> 5 -> END
        uint32_t fat_sec = ((struct fat_private*)drive1.fs_private)->bpb.reserved_sectors;
        uint16_t* fat_buf = (uint16_t*)kmalloc(512);
        ata_read_sector(1, fat_sec, fat_buf);
        fat_buf[4] = 5;
        fat_buf[5] = 0xFFFF;
        ata_write_sector(1, fat_sec, fat_buf);
        kfree(fat_buf);

        file_entry->filesize = 1011; 
        
        // Write directory AFTER updating all entries
        ata_write_sector(1, dir_sec, dir_buf);

        serial_print("Injection complete. Attempting to open 1:/DIR/TEST.TXT via VFS...\n");
        
        fs_init();
        fs_insert_filesystem(fat16_init_vfs());
        
        int fd = fopen("1:/DIR/TEST.TXT", "r");
        if (fd > 0) {
            serial_print("Opened successfully!\n");
            char read_back[32];
            
            // Read start
            memset(read_back, 0, 32);
            fread(read_back, 1, 19, fd);
            serial_print("Start Content: ");
            serial_print(read_back);
            serial_print("\n");

            // Seek to near end (manual seek for now)
            int sres = fseek(fd, 1000, FILE_SEEK_SET);
            if (sres != 0) serial_print("FSEEK SET FAILED!\n");
            memset(read_back, 0, 32);
            fread(read_back, 1, 11, fd);
            serial_print("End Content: ");
            serial_print(read_back);
            serial_print("\n");

            // Test SEEK_CUR
            fseek(fd, -11, FILE_SEEK_CUR);
            memset(read_back, 0, 32);
            fread(read_back, 1, 11, fd);
            serial_print("Re-read End Content (CUR): ");
            serial_print(read_back);
            serial_print("\n");

            // Test ftell
            int pos = ftell(fd);
            if (pos != 1011) serial_print("FTELL FAILED!\n");
            else serial_print("FTELL SUCCESS!\n");

            // Test fstat
            struct file_stat stat;
            if (fstat(fd, &stat) == 0) {
                serial_print("FSTAT SUCCESS: Size=");
                // Since I don't have a number printer, I'll just check if it's correct
                if (stat.filesize == 1011) serial_print("Correct\n");
                else serial_print("Incorrect\n");
            } else {
                serial_print("FSTAT FAILED!\n");
            }

            fclose(fd);
        } else {
            serial_print("Failed to open 1:/DIR/TEST.TXT via VFS. Code: ");
            if (fd < 0) {
                uint32_t val = -fd;
                serial_putc(val + '0');
            }
            serial_print("\n");
        }

        serial_print("Testing robustness: Opening non-existent file 1:/NONEXIST.TXT...\n");
        int fd_bad = fopen("1:/NONEXIST.TXT", "r");
        if (fd_bad < 0) {
            serial_print("VFS correctly rejected non-existent file. Error code: -");
            serial_putc((-fd_bad) + '0');
            serial_print("\n");
        } else {
            serial_print("VFS INCORRECTLY opened non-existent file!\n");
            fclose(fd_bad);
        }

        serial_print("Testing robustness: Opening directory 1:/DIR as a file...\n");
        int fd_dir = fopen("1:/DIR", "r");
        if (fd_dir < 0) {
            serial_print("VFS correctly rejected opening directory as file.\n");
        } else {
            serial_print("VFS INCORRECTLY opened directory as file!\n");
            fclose(fd_dir);
        }
    } else {
        serial_print("FAT16 Initialization FAILED! Code: ");
        if (fat_res < 0) serial_putc('-');
        uint32_t val = (fat_res < 0) ? -fat_res : fat_res;
        serial_putc(val + '0');
        serial_print("\n");
    }

    serial_print("Kernel execution finished. Hanging system...\n");
    while(1);
}
