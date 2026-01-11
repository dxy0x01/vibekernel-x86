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

    // 3. Identity Map First 64MB (Kernel, Stack, Video, Heap, BIOS)
    // We map this with provided flags (usually present/writeable)
    // BUT we don't necessarily want User access for all of it.
    // However, for now, let's keep it simple and use provided flags.
    for (int i = 0; i < 16; i++) {
        uint32_t* table = kmalloc_a(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE; b++) {
            table[b] = ((uint32_t)i * PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE * PAGING_PAGE_SIZE + (b * PAGING_PAGE_SIZE)) | flags;
        }
        // The PDE itself should allow User access so that PTEs can decide.
        // We always add PAGING_ACCESS_FROM_ALL to the PDE.
        directory[i] = (uint32_t)table | flags | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL;
    }

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

int paging_is_aligned(void* addr)
{
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

void paging_flush_tlb_single(uint32_t addr)
{
    __asm__ __volatile__("invlpg (%0)" :: "r" (addr) : "memory");
}

int paging_set(uint32_t* directory, void* virt, uint32_t val)
{
    if (!paging_is_aligned(virt))
    {
        return -1;
    }

    uint32_t directory_index = ((uint32_t)virt) >> 22;
    uint32_t table_index = (((uint32_t)virt) >> 12) & 0x3FF;

    uint32_t entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);

    if (!table)
    {
        // Allocate a new page table
        table = kmalloc_a(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE);
        for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE_SIZE; i++)
        {
            table[i] = 0;
        }

        // Add the table to the directory.
        // We set PAGING_ACCESS_FROM_ALL here so that individual PTEs can control access.
        directory[directory_index] = (uint32_t)table | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL;
    }

    table[table_index] = val;

    // Flush TLB
    paging_flush_tlb_single((uint32_t)virt);

    return 0;
}
