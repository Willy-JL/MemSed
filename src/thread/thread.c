#define _GNU_SOURCE
#include "thread.h"

#include <errno.h>
#include <pthread.h>

// Linux: pthreads
struct Thread {
    pthread_t tid;
};

Thread* thread_start(ThreadCallback callback, void* context) {
    Thread* thread = malloc(sizeof(Thread));
    int32_t res = pthread_create(&thread->tid, NULL, callback, context);
    if(res != 0) {
        errno = res;
        perror("pthread_create");
        free(thread);
        return NULL;
    }
    return thread;
}

void thread_stop(Thread* thread) {
    int32_t res = pthread_cancel(thread->tid);
    if(res != 0) {
        errno = res;
        perror("pthread_cancel");
    }
}

bool thread_tryjoin(Thread* thread, void** result) {
    void* ret;
    int32_t res = pthread_tryjoin_np(thread->tid, &ret);
    if(res != 0) {
        if(res == EBUSY) {
            return false;
        }
        errno = res;
        perror("pthread_tryjoin_np");
        return false;
    }
    if(ret == PTHREAD_CANCELED) {
        ret = NULL;
    }
    if(result != NULL) {
        *result = ret;
    }
    free(thread);
    return true;
}
