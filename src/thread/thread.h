#pragma once

#include <std.h>

// Linux: pthreads
#include <pthread.h>

// Linux: useconds_t
#include <unistd.h>

typedef struct Thread Thread;

typedef void* (*ThreadCallback)(void* context);
typedef void (*ThreadCancelCleanup)(void* context);

Thread* thread_start(ThreadCallback callback, void* context);
void thread_cancel(Thread* thread);
bool thread_try_join(Thread* thread, void** result);
void thread_join(Thread* thread, void** result);
void thread_self_enable_canceling();
void thread_self_disable_canceling();
void thread_self_quit_if_canceled();
void thread_self_usleep(useconds_t useconds);
#define thread_self_push_cancel_cleanup(cleanup, context) pthread_cleanup_push(cleanup, context)
#define thread_self_pop_cancel_cleanup(execute)           pthread_cleanup_pop(execute)
