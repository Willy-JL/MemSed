#define _GNU_SOURCE
#include "thread.h"

// Linux: pthreads, notify with eventfd, sleep with poll
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>

typedef struct {
    ThreadCancelCleanupCb callback;
    void* context;
} ThreadCancelCleanup;

struct Thread {
    pthread_t tid;
    ThreadCallback callback;
    void* context;
    bool canceled;
    int32_t canceled_eventfd;
    size_t cleanups_count;
    ThreadCancelCleanup* cleanups;
};

static void* thread_body(void* context) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    Thread* self = context;
    return self->callback(self, self->context);
}

Thread* thread_start(ThreadCallback callback, void* context) {
    Thread* thread = malloc(sizeof(Thread));
    thread->callback = callback;
    thread->context = context;
    thread->canceled = false;
    thread->canceled_eventfd = eventfd(0, EFD_NONBLOCK);
    if(thread->canceled_eventfd < 0) {
        perror("eventfd()");
        free(thread);
        return NULL;
    }
    thread->cleanups_count = 0;
    thread->cleanups = NULL;
    int32_t res = pthread_create(&thread->tid, NULL, thread_body, thread);
    if(res != 0) {
        errno = res;
        perror("pthread_create()");
        close(thread->canceled_eventfd);
        free(thread);
        return NULL;
    }
    return thread;
}

void thread_cancel(Thread* thread) {
    thread->canceled = true;
    uint64_t notify = 1;
    ssize_t res = write(thread->canceled_eventfd, &notify, sizeof(notify));
    if(res < 0 && errno != EAGAIN) {
        perror("write(eventfd)");
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
    close(thread->canceled_eventfd);
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
    close(thread->canceled_eventfd);
    free(thread);
    return;
}

static void thread_assert_self(Thread* self) {
    UNUSED(self); // Assert is optimized-out on Release builds
    assert(pthread_equal(self->tid, pthread_self()));
}

void thread_self_quit_if_canceled(Thread* self) {
    thread_assert_self(self);
    if(!self->canceled) return;

    if(self->cleanups_count > 0) {
        for(size_t i = self->cleanups_count - 1; i < self->cleanups_count; i--) {
            ThreadCancelCleanup* cleanup = &self->cleanups[i];
            cleanup->callback(cleanup->context);
        }
        self->cleanups_count = 0;
        free(self->cleanups);
        self->cleanups = NULL;
    }

    pthread_exit(PTHREAD_CANCELED);
}

void thread_self_usleep(Thread* self, size_t usec) {
    thread_assert_self(self);

    struct timespec timeout = {
        .tv_sec = 0,
        .tv_nsec = usec * 1000,
    };
    if(usec >= 1'000'000) {
        div_t sec = div(usec, 1'000'000);
        timeout.tv_sec = sec.quot;
        timeout.tv_nsec = sec.rem * 1000;
    }

    struct pollfd canceled = {
        .fd = self->canceled_eventfd,
        .events = POLLIN,
    };

    ppoll(&canceled, 1, &timeout, NULL);
}

void thread_self_push_cancel_cleanup(Thread* self, ThreadCancelCleanupCb callback, void* context) {
    thread_assert_self(self);

    if(self->cleanups_count > 0) {
        self->cleanups =
            realloc(self->cleanups, sizeof(ThreadCancelCleanup) * (self->cleanups_count + 1));
    } else {
        self->cleanups = malloc(sizeof(ThreadCancelCleanup) * 1);
    }

    ThreadCancelCleanup* cleanup = &self->cleanups[self->cleanups_count];
    cleanup->callback = callback;
    cleanup->context = context;
    self->cleanups_count++;
}

void thread_self_pop_cancel_cleanup(Thread* self, bool execute) {
    thread_assert_self(self);

    assert(self->cleanups_count > 0);
    self->cleanups_count--;

    if(execute) {
        ThreadCancelCleanup* cleanup = &self->cleanups[self->cleanups_count];
        cleanup->callback(cleanup->context);
    }

    if(self->cleanups_count > 0) {
        self->cleanups =
            realloc(self->cleanups, sizeof(ThreadCancelCleanup) * self->cleanups_count);
    } else {
        free(self->cleanups);
        self->cleanups = NULL;
    }
}
