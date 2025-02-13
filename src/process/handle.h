#pragma once

#include "pid.h"

typedef struct ProcessHandle ProcessHandle;

ProcessHandle* process_handle_init(ProcessPid pid);
bool process_handle_is_valid(ProcessHandle* handle);
void process_handle_free(ProcessHandle* handle);
