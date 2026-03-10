#pragma once

#include "pid.h"

typedef struct {
    ProcessPid pid;
    char name[16];
    char executable[256];
    char command[256];
    char user[256];
} Process;

Process* process_init(ProcessPid pid);
void process_free(Process* process);
