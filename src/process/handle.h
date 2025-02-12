#pragma once

#include "process.h"

// Linux: file handle to /proc/pid/mem
typedef FILE* ProcessHandleImpl;

typedef struct {
    Process* process;
    ProcessHandleImpl impl;
} ProcessHandle;

ProcessHandle* process_handle_init(Process* process);
bool process_handle_is_valid(ProcessHandle* handle);
void process_handle_free(ProcessHandle* handle);
