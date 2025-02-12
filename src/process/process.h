#pragma once

#include <std.h>

// Linux: unsigned, 2^15 on 32-bit, 2^22 on 64-bit
// Windows: signed, 2^32
// Invalid PID is 0
typedef int32_t ProcessPid;

typedef struct {
    ProcessPid pid;
    char name[16];
    char* executable;
    char* command;
    char* user;
} Process;

Process* process_init(ProcessPid pid);
bool process_is_alive(Process* process);
void process_free(Process* process);
