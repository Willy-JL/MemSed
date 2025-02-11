#pragma once

#include <std.h>

// Linux: unsigned, 2^15 on 32-bit, 2^22 on 64-bit
// Windows: signed, 2^32
// Invalid PID is 0
typedef int32_t MemoryProcessPid;

typedef struct {
    MemoryProcessPid pid;
    char* name;
    char* command;
    char* user;
} MemoryProcess;

MemoryProcess* memory_process_init(MemoryProcessPid pid);
bool memory_process_is_alive(MemoryProcess* memory_process);
void memory_process_free(MemoryProcess* memory_process);
