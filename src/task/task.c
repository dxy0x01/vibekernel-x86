#include "task.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include "../kernel/panic.h"
#include "process.h"
#include <stddef.h>

struct task* current_task = NULL;
struct task* task_tail = NULL;
struct task* task_head = NULL;

int task_init(struct task* task, struct process* process) {
    memset(task, 0, sizeof(struct task));
    task->process = process;

    // Allocate User Stack (16KB)
    task->user_stack = kmalloc(16384);
    if (!task->user_stack) return -1;

    memset(task->user_stack, 0, 16384);

    // Allocate Kernel Stack (16KB)
    task->kstack = kmalloc(16384);
    if (!task->kstack) {
        kfree(task->user_stack);
        return -1;
    }
    memset(task->kstack, 0, 16384);

    uint32_t stack_top = (uint32_t)task->user_stack + 16383;
    
    // Initialize registers for User Mode
    task->regs.ss = 0x23; // User Data (0x20 | 3)
    task->regs.esp = stack_top;
    task->regs.cs = 0x1B; // User Code (0x18 | 3)
    task->regs.eip = (uint32_t)process->ptr;
    task->regs.eflags = 0x202; // IF | Reserved

    // Map user stack in the process's page directory
    // For now, identity map it but with User access
    uint32_t stack_page = (uint32_t)task->user_stack & 0xFFFFF000;
    for (uint32_t i = 0; i < 16384; i += PAGING_PAGE_SIZE) {
        paging_set(process->paging_chunk->directory_entry, (void*)(stack_page + i), (stack_page + i) | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_ACCESS_FROM_ALL);
    }

    return 0;
}

struct task* task_new(struct process* process) {
    struct task* task = kmalloc(sizeof(struct task));
    if (!task) return NULL;

    if (task_init(task, process) != 0) {
        kfree(task);
        return NULL;
    }

    if (!task_head) {
        task_head = task;
        task_tail = task;
        current_task = task;
        process_switch(process);
    } else {
        task_tail->next = task;
        task->prev = task_tail;
        task_tail = task;
    }

    return task;
}

struct task* task_current() {
    return current_task;
}

void task_free(struct task* task) {
    if (task->prev) task->prev->next = task->next;
    if (task->next) task->next->prev = task->prev;
    if (task == task_head) task_head = task->next;
    if (task == task_tail) task_tail = task->prev;
    if (task == current_task) current_task = task_head;
    
    if (task->user_stack) kfree(task->user_stack);
    if (task->kstack) kfree(task->kstack);
    kfree(task);
}

extern tss_entry_t tss_entry;
void task_switch(struct task* task) {
    current_task = task;
    process_switch(task->process);
    paging_switch(task->process->paging_chunk->directory_entry);
    tss_entry.esp0 = (uint32_t)task->kstack + 16383;
    task_return(&task->regs);
}

int task_copy_string_from_user(struct task* task, void* virtual, char* phys, int max) {
    if (max <= 0) return -1;

    uint32_t* old_directory = task_current()->process->paging_chunk->directory_entry;
    paging_switch(task->process->paging_chunk->directory_entry);

    char* v = (char*)virtual;
    int i = 0;
    for (i = 0; i < max - 1; i++) {
        phys[i] = v[i];
        if (v[i] == '\0') break;
    }
    phys[i] = '\0';

    paging_switch(old_directory);
    return i;
}

uint32_t task_get_stack_item(struct task* task, int index) {
    uint32_t* old_directory = task_current()->process->paging_chunk->directory_entry;
    paging_switch(task->process->paging_chunk->directory_entry);

    uint32_t* sp = (uint32_t*)task->regs.esp;
    uint32_t item = sp[index];

    paging_switch(old_directory);
    return item;
}
