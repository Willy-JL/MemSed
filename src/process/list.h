#pragma once

#include "process.h"

typedef struct {
    size_t processes_count;
    Process* processes[];
} ProcessList;

ProcessList* process_list_init(void);
void process_list_free(ProcessList* list);
