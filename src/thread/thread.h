#pragma once

#include <std.h>

// Linux: pthreads
typedef pthread_t ThreadImpl;

typedef struct {
    ThreadImpl impl;
} Thread;

typedef void* (*ThreadCallback)(void* context);

Thread* thread_start(ThreadCallback callback, void* context);
void thread_stop(Thread* thread);
bool thread_tryjoin(Thread* thread, void** result);
