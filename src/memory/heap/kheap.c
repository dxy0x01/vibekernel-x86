#include "kheap.h"
#include "../../drivers/screen.h"

extern uint32_t end; // Defined in linker.ld
uint32_t placement_address = (uint32_t)&end;

#define HEAP_SIZE 1024 * 1024 * 4 // 4MB Heap (Enough for Paging Structures)

typedef struct block_meta {
    size_t size;
    struct block_meta *next;
    int free;
    int magic;
} block_meta_t;

block_meta_t *global_base = NULL;

block_meta_t *request_space(block_meta_t* last, size_t size);
block_meta_t *find_free_block(block_meta_t **last, size_t size);

void kheap_init() {
    print_string("Initializing Heap...\n");
    // placement_address is already set to &end
}

// Internal function to handle allocation logic
void *kmalloc_int(uint32_t size, int align, uint32_t *phys) {
    block_meta_t *block;

    if (size <= 0) return NULL;

    if (!global_base) { // First call
        if (align == 1) {
            uint32_t addr = placement_address + sizeof(block_meta_t);
            if (addr & 0xFFF) {
                placement_address &= 0xFFFFF000;
                placement_address += 0x1000;
                placement_address -= sizeof(block_meta_t);
            }
        }
        
        block = request_space(NULL, size);
        if (!block) return NULL;
        global_base = block;
    } else {
        block_meta_t *last = global_base;
        block = find_free_block(&last, size);
        
        if (block && align) {
            uint32_t addr = (uint32_t)(block + 1);
            if (addr & 0xFFF) {
                block = NULL; // Force new allocation if block not aligned
            }
        }

        if (!block) { // Need new space
            if (align == 1) {
                uint32_t addr = placement_address + sizeof(block_meta_t);
                if (addr & 0xFFF) {
                    placement_address &= 0xFFFFF000;
                    placement_address += 0x1000;
                    placement_address -= sizeof(block_meta_t);
                }
            }
            block = request_space(last, size);
            if (!block) return NULL;
        } else {
            block->free = 0;
            block->magic = 0x77777777;
        }
    }
    
    void* v_addr = (void*)(block + 1);
    
    if (phys) {
        *phys = (uint32_t)v_addr; // Identity mapped initially
    }
    
    return v_addr;
}

block_meta_t *find_free_block(block_meta_t **last, size_t size) {
    block_meta_t *current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

block_meta_t *request_space(block_meta_t* last, size_t size) {
    block_meta_t *block;
    block = (block_meta_t *)placement_address;

    // TODO: Check against Heap Size limit?
    // if (placement_address + size + sizeof(block_meta_t) > HEAP_LIMIT) return NULL;

    placement_address += size + sizeof(block_meta_t);

    if (last) {
        last->next = block;
    }
    
    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->magic = 0x12345678;
    return block;
}

void *kmalloc(uint32_t size) {
    return kmalloc_int(size, 0, NULL);
}

void *kmalloc_a(uint32_t size) {
    return kmalloc_int(size, 1, NULL);
}

void *kmalloc_p(uint32_t size, uint32_t *phys) {
    return kmalloc_int(size, 0, phys);
}

void *kmalloc_ap(uint32_t size, uint32_t *phys) {
    return kmalloc_int(size, 1, phys);
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_meta_t *block = (block_meta_t*)ptr - 1;
    if (block->magic != 0x12345678 && block->magic != 0x77777777) return;
    block->free = 1;
}
