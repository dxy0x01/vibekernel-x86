#include "process.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include <stddef.h>

struct process* process_head = NULL;
struct process* process_tail = NULL;
static struct process* current_process = NULL;

static int process_get_free_slot() {
    static int current_id = 0;
    return ++current_id;
}

int process_alloc(struct process** process) {
    int res = 0;
    struct process* proc = kmalloc(sizeof(struct process));
    if (!proc) {
        res = -1; // ENOMEM
        goto out;
    }

    memset(proc, 0, sizeof(struct process));
    proc->id = process_get_free_slot();

    if (!process_head) {
        process_head = proc;
        process_tail = proc;
    } else {
        process_tail->next = proc;
        process_tail = proc;
    }

    *process = proc;

out:
    return res;
}

#include "../fs/file.h"

#include "../loader/elf.h"

int process_load(const char* filename, struct process** process) {
    int res = 0;
    int fd = fopen(filename, "r");
    if (fd < 0) return -1;

    uint32_t magic = 0;
    if (fread(&magic, 4, 1, fd) != 1) {
        fclose(fd);
        return -1;
    }
    fclose(fd);

    if (magic == 0x464C457F) { // .ELF in little endian
        return elf_load(filename, process);
    }

    // Raw binary loading fallback
    struct process* proc = NULL;
    res = process_alloc(&proc);
    if (res < 0) goto out;

    strcpy(proc->name, filename);
    
    fd = fopen(filename, "r");
    if (fd < 0) {
        res = -1;
        goto out;
    }
    // ... (rest of raw loading logic)
    struct file_stat stat;
    if (fstat(fd, &stat) != 0) {
        res = -1;
        goto out;
    }

    proc->size = stat.filesize;
    proc->ptr = kmalloc_a(proc->size);
    if (!proc->ptr) {
        res = -1;
        goto out;
    }

    if (fread(proc->ptr, proc->size, 1, fd) != 1) {
        res = -1;
        goto out;
    }

    fclose(fd);

    // Initial paging setup for process
    proc->paging_chunk = paging_new_4gb(PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);
    if (!proc->paging_chunk) {
        res = -1;
        goto out;
    }

    for (int i = 0; i < proc->size; i += PAGING_PAGE_SIZE) {
        uint32_t val = ((uint32_t)proc->ptr + i) | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL;
        paging_set(proc->paging_chunk->directory_entry, (void*)(0x400000 + i), val);
    }

    paging_set(proc->paging_chunk->directory_entry, (void*)0xB8000, 0xB8000 | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);

    proc->ptr = (void*)0x400000;
    *process = proc;

out:
    return res;
}

void process_free(struct process* process) {
    // Basic cleanup logic (to be expanded with paging and task cleanup)
    if (process == process_head) {
        process_head = process->next;
    } else {
        struct process* p = process_head;
        while (p && p->next != process) p = p->next;
        if (p) p->next = process->next;
    }

    if (process == process_tail) {
        struct process* p = process_head;
        while (p && p->next) p = p->next;
        process_tail = p;
    }

    kfree(process);
}

struct process* process_get(int process_id) {
    struct process* proc = process_head;
    while (proc) {
        if (proc->id == process_id) return proc;
        proc = proc->next;
    }
    return NULL;
}

struct process* process_current() {
    return current_process;
}

int process_switch(struct process* process) {
    current_process = process;
    return 0;
}
