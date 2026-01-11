#include "../drivers/screen.h"
#include "../cpu/isr.h"
#include "../drivers/serial.h"
#include "../drivers/ata.h"

#include "../memory/heap/kheap.h"
#include "../memory/paging/paging.h"

// Simulate idt_init if it doesn't match exactly yet
void idt_init() {
    isr_install();
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void main() {
    serial_init();
    serial_print("Serial Initialized.\n");

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
    
    __asm__ __volatile__("sti");

    print_string("Paging Initialized.\n");
    
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
    // Manual hex printing (since print_hex is in screen.h but not exposed? or is it?)
    // Actually print_hex was in screen.h?
    // I should check screen.h content.
    // Assuming print_hex is available.
    // print_hex((uint32_t)ptr1);
    // print_string("\n");
    
    // Check bits 0-11 are zero
    if (((uint32_t)ptr1 & 0xFFF) == 0) {
        print_string("Alignment Success!\n");
        serial_print("Alignment Success!\n");
    } else {
        print_string("Alignment FAILED!\n");
        serial_print("Alignment FAILED!\n");
    }

    // Test ATA Disk Read/Write/Identify
    serial_print("Testing Robust ATA Disk Driver...\n");
    
    if (ata_identify() == 0) {
        serial_print("Drive IDENTIFY Successful!\n");
    } else {
        serial_print("Drive IDENTIFY FAILED!\n");
    }

    uint16_t* test_buffer = (uint16_t*)kmalloc(512);
    // Fill with pattern
    for(int i=0; i<256; i++) test_buffer[i] = 0xAA55; // Distinct pattern

    serial_print("Writing pattern 0xAA55 to Sector 2...\n");
    int res = ata_write_sector(2, test_buffer);
    if (res == 0) {
        serial_print("Write Successful.\n");
    } else {
        serial_print("Write FAILED! Code: ");
        serial_putc((res*-1) + '0');
        serial_print("\n");
    }

    // Clear buffer
    for(int i=0; i<256; i++) test_buffer[i] = 0;

    serial_print("Reading back Sector 2...\n");
    res = ata_read_sector(2, test_buffer);
    if (res == 0) {
        if (test_buffer[0] == 0xAA55) {
            serial_print("Verification Success: Read back 0xAA55 from Sector 2!\n");
        } else {
            serial_print("Verification FAILED: Pattern mismatch! Read: ");
            // Print first word
            uint16_t val = test_buffer[0];
            for(int j=0; j<4; j++) {
                char h = (val >> (12-j*4)) & 0xF;
                serial_putc(h < 10 ? h + '0' : h - 10 + 'A');
            }
            serial_print("\n");
        }
    } else {
        serial_print("Read back FAILED! Code: ");
        serial_putc((res*-1) + '0');
        serial_print("\n");
    }
}
