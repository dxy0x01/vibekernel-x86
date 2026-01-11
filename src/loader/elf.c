#include "elf.h"
#include "../fs/file.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include "../task/process.h"
#include "../memory/paging/paging.h"
#include <stddef.h>

bool elf_is_valid_header(Elf32_Ehdr* header) {
    return header->e_ident[0] == ELFMAG0 &&
           header->e_ident[1] == ELFMAG1 &&
           header->e_ident[2] == ELFMAG2 &&
           header->e_ident[3] == ELFMAG3 &&
           header->e_ident[4] == ELFCLASS32;
}

void* elf_get_entry_point(Elf32_Ehdr* header) {
    return (void*)header->e_entry;
}

int elf_load(const char* filename, struct process** process) {
    int res = 0;
    int fd = fopen(filename, "r");
    if (fd < 0) return -1;

    Elf32_Ehdr header;
    if (fread(&header, sizeof(Elf32_Ehdr), 1, fd) != 1) {
        res = -1;
        goto out;
    }

    if (!elf_is_valid_header(&header)) {
        res = -1;
        goto out;
    }

    struct process* proc = NULL;
    res = process_alloc(&proc);
    if (res < 0) goto out;

    strcpy(proc->name, filename);
    proc->paging_chunk = paging_new_4gb(PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);

    // Map VGA
    paging_set(proc->paging_chunk->directory_entry, (void*)0xB8000, 0xB8000 | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);

    // Load program segments
    fseek(fd, header.e_phoff, FILE_SEEK_SET);
    for (int i = 0; i < header.e_phnum; i++) {
        Elf32_Phdr phdr;
        if (fread(&phdr, sizeof(Elf32_Phdr), 1, fd) != 1) {
            res = -1;
            goto out;
        }

        if (phdr.p_type == PT_LOAD) {
            void* phys_ptr = kmalloc_a(phdr.p_memsz);
            memset(phys_ptr, 0, phdr.p_memsz);

            long current_pos = ftell(fd);
            fseek(fd, phdr.p_offset, FILE_SEEK_SET);
            if (fread(phys_ptr, phdr.p_filesz, 1, fd) != 1) {
                kfree(phys_ptr);
                res = -1;
                goto out;
            }
            fseek(fd, current_pos, FILE_SEEK_SET);

            // Map segment
            for (int b = 0; b < phdr.p_memsz; b += PAGING_PAGE_SIZE) {
                uint32_t val = ((uint32_t)phys_ptr + b) | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL;
                paging_set(proc->paging_chunk->directory_entry, (void*)((uint32_t)phdr.p_vaddr + b), val);
            }
        }
    }

    proc->ptr = (void*)header.e_entry; // This is a bit misleading in the struct but okay for now
    *process = proc;

out:
    fclose(fd);
    return res;
}
