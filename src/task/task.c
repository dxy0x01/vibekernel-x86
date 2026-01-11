#include "task.h"
#include "../memory/heap/kheap.h"
#include "../string/string.h"
#include "../kernel/panic.h"
#include <stddef.h>

struct task* current_task = NULL;
struct task* task_tail = NULL;
struct task* task_head = NULL;

int task_init(struct task* task, struct process* process) {
    memset(task, 0, sizeof(struct task));
    task->process = process;
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
    // Basic cleanup logic (to be expanded)
    if (task->prev) task->prev->next = task->next;
    if (task->next) task->next->prev = task->prev;
    if (task == task_head) task_head = task->next;
    if (task == task_tail) task_tail = task->prev;
    if (task == current_task) current_task = task_head;
    
    kfree(task);
}
