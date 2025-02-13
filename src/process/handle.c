#include "handle.h"

// Linux: file handle to /proc/pid/mem
struct ProcessHandle {
    ProcessPid pid;
    FILE* mem;
};

ProcessHandle* process_handle_init(ProcessPid pid) {
    ProcessHandle* handle = malloc(sizeof(ProcessHandle));
    handle->pid = pid;
    char path[31];
    snprintf(path, sizeof(path), "/proc/%i/mem", pid);
    handle->mem = fopen(path, "r+");
    if(handle->mem == NULL) {
        perror(path);
        free(handle);
        return NULL;
    }
    return handle;
}

bool process_handle_is_valid(ProcessHandle* handle) {
    return ftell(handle->mem) >= 0 && process_pid_is_alive(handle->pid);
}

void process_handle_free(ProcessHandle* handle) {
    fclose(handle->mem);
    free(handle);
}
