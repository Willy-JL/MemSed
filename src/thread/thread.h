#pragma once

#include <std.h>

typedef struct Thread Thread;

typedef void* (*ThreadCallback)(void* context);

Thread* thread_start(ThreadCallback callback, void* context);
void thread_stop(Thread* thread);
bool thread_tryjoin(Thread* thread, void** result);
