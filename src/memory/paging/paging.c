#include "paging.h"
#include "../heap/kheap.h"

extern void print_string(char* message);

void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    // 1. Allocate Page Directory (Aligned 4KB)
    uint32_t* directory = kmalloc_a(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE);
    
    // 2. Clear Directory (Mark all Not Present by default)
    for (int i=0; i<PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE; i++) {
        directory[i] = 0; // Not Present (Bit 0 = 0)
    }

    // 3. Identity Map First 4MB (Kernel, Stack, Video, Heap Start)
    // We only allocate ONE Page Table for now.
    // This saves massive heap space compared to mapping all 4GB.
    
    uint32_t* entry = kmalloc_a(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE);
    // Fill the table: Map 0x00000000 -> 0x003FFFFF
    int offset = 0;
    for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE; b++)
    {
        entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
    }
    
    // Add this table to Directory Index 0
    directory[0] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE; // Directory entry needs RW

    struct paging_4gb_chunk* chunk_4gb = kmalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

void paging_switch(uint32_t* directory)
{
    paging_load_directory(directory);
    current_directory = directory;
}

void paging_load_directory(uint32_t* directory)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r" (directory));
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return chunk->directory_entry;
}

void enable_paging()
{
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0":: "r"(cr0));
}
