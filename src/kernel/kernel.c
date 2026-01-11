#include "../drivers/screen.h"
#include "../cpu/isr.h"

#include "../memory/heap/kheap.h"
#include "../memory/paging/paging.h"

// Simulate idt_init if it doesn't match exactly yet
void idt_init() {
    isr_install();
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void main() {
    clear_screen();
    print_string("Hello World from C Kernel!\n");
    
    idt_init();
    kheap_init();
    
    // Setup paging
    // flags: RW | Present | Access from all (User)
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    // Switch to kernel paging chunk
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    // Enable paging
    enable_paging();
    
    __asm__ __volatile__("sti");

    print_string("Paging Initialized.\n");
    
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
    } else {
        print_string("Alignment FAILED!\n");
    }
}
