#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "task.h"

#define MAX_PROCESS_FILES 10

struct process {
    uint16_t id;
    char name[32];
    struct task* task;
    void* file_descriptors[MAX_PROCESS_FILES];
    struct paging_4gb_chunk* paging_chunk;
    struct process* next;
};

int process_alloc(struct process** process);
int process_load(const char* filename, struct process** process);
void process_free(struct process* process);
struct process* process_get(int process_id);
int process_switch(struct process* process);

#endif
