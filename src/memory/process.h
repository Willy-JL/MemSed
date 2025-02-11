#pragma once

#include <std.h>

// Linux: unsigned, 2^15 on 32-bit, 2^22 on 64-bit
// Windows: signed, 2^32
// Invalid PID is 0
typedef int32_t MemoryProcessPid;

char* memory_process_get_description(MemoryProcessPid pid);
