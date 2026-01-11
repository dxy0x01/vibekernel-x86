#include "mem.h"
#include "../drivers/screen.h"
#include <stddef.h>
#include <stdint.h>

extern uint32_t end; // Defined in linker.ld
uint32_t placement_address = (uint32_t)&end;

#define HEAP_START placement_address
#define HEAP_SIZE 100 * 1024 // 100KB heap

typedef struct block_meta {
    size_t size;
    struct block_meta *next;
    int free;
    int magic;
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)

block_meta_t *global_base = NULL;

// Forward declarations
block_meta_t *find_free_block(block_meta_t **last, size_t size);
block_meta_t *request_space(block_meta_t* last, size_t size);

void memory_init() {
    print_string("Initializing Memory Manager...\n");
    print_string("Kernel End: 0x");
    // We need a hex printer! But sticking to placement for now.
    // Ideally we print (uint32_t)&end
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

    // Check header overflow? (Assuming sufficient heap for now)
    
    // Increment placement address
    if (placement_address + size + META_SIZE > (uint32_t)&end + HEAP_SIZE) {
        return NULL; // Out of memory
    }
    
    placement_address += size + META_SIZE;

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
    block_meta_t *block;

    if (size <= 0) return NULL;

    if (!global_base) { // First call
        block = request_space(NULL, size);
        if (!block) return NULL;
        global_base = block;
    } else {
        block_meta_t *last = global_base;
        block = find_free_block(&last, size);
        if (!block) { // Failed to find free block
            block = request_space(last, size);
            if (!block) return NULL;
        } else {      // Found free block
            // TODO: split block if too large
            block->free = 0;
            block->magic = 0x77777777;
        }
    }
    
    return (void *)(block + 1);
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_meta_t *block = (block_meta_t*)ptr - 1;
    if (block->magic != 0x12345678 && block->magic != 0x77777777) {
        // Validation failed
        return;
    }
    block->free = 1;
    // TODO: merge free blocks
}
