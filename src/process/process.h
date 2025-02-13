#pragma once

#include "pid.h"

typedef struct {
    ProcessPid pid;
    char name[16];
    char* executable;
    char* command;
    char* user;
} Process;

Process* process_init(ProcessPid pid);
void process_free(Process* process);
