#define _GNU_SOURCE
#include "thread.h"

#include <errno.h>
#include <pthread.h>

// Linux: pthreads
struct Thread {
    pthread_t tid;
    ThreadCallback callback;
    void* context;
};

static void* thread_body(void* context) {
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    Thread* thread = context;
    return thread->callback(thread->context);
}

Thread* thread_start(ThreadCallback callback, void* context) {
    Thread* thread = malloc(sizeof(Thread));
    thread->callback = callback;
    thread->context = context;
    int32_t res = pthread_create(&thread->tid, NULL, thread_body, thread);
    if(res != 0) {
        errno = res;
        perror("pthread_create()");
        free(thread);
        return NULL;
    }
    return thread;
}

void thread_cancel(Thread* thread) {
    int32_t res = pthread_cancel(thread->tid);
    if(res != 0) {
        errno = res;
        perror("pthread_cancel()");
    }
}

bool thread_try_join(Thread* thread, void** result) {
    void* ret;
    int32_t res = pthread_tryjoin_np(thread->tid, &ret);
    if(res != 0) {
        if(res == EBUSY) {
            return false;
        }
        errno = res;
        perror("pthread_tryjoin_np()");
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

void thread_join(Thread* thread, void** result) {
    void* ret;
    int32_t res = pthread_join(thread->tid, &ret);
    if(res != 0) {
        errno = res;
        perror("pthread_join()");
        *result = NULL;
        return;
    }
    if(ret == PTHREAD_CANCELED) {
        ret = NULL;
    }
    if(result != NULL) {
        *result = ret;
    }
    free(thread);
    return;
}

void thread_self_enable_canceling(void) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

void thread_self_disable_canceling(void) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
}

void thread_self_quit_if_canceled(void) {
    int32_t old_state;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);
    pthread_testcancel();
    if(old_state == PTHREAD_CANCEL_DISABLE) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }
}

void thread_self_usleep(useconds_t usec) {
    usleep(usec);
}
