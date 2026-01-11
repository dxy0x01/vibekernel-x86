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
};

#endif
