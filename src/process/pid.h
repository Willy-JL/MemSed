#pragma once

#include <std.h>

// Linux: unsigned, 2^15 on 32-bit, 2^22 on 64-bit
// Windows: signed, 2^32
// Invalid PID is 0
typedef int32_t ProcessPid;

bool process_pid_is_alive(ProcessPid pid);
void process_pid_pause(ProcessPid pid);
void process_pid_resume(ProcessPid pid);
