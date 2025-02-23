#pragma once

#include <std.h>

typedef struct Thread Thread;

typedef void* (*ThreadCallback)(Thread* self, void* context);
typedef void (*ThreadCancelCleanupCb)(void* context);

Thread* thread_start(ThreadCallback callback, void* context);
void thread_cancel(Thread* thread);
bool thread_try_join(Thread* thread, void** result);
void thread_join(Thread* thread, void** result);
void thread_self_quit_if_canceled(Thread* self);
void thread_self_usleep(Thread* self, size_t useconds);
void thread_self_push_cancel_cleanup(Thread* self, ThreadCancelCleanupCb callback, void* context);
void thread_self_pop_cancel_cleanup(Thread* self, bool execute);
