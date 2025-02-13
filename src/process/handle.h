#pragma once

#include "../memory/address.h"
#include "pid.h"

typedef struct ProcessHandle ProcessHandle;

ProcessHandle* process_handle_init(ProcessPid pid);
bool process_handle_is_valid(ProcessHandle* handle);
size_t process_handle_read(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size);
size_t process_handle_write(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size);
void process_handle_free(ProcessHandle* handle);
