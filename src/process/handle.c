#include "handle.h"

ProcessHandle* process_handle_init(Process* process) {
    ProcessHandle* handle = malloc(sizeof(ProcessHandle));
    handle->process = process;
    char path[31];
    snprintf(path, sizeof(path), "/proc/%i/mem", process->pid);
    handle->impl = fopen(path, "r+");
    if(handle->impl == NULL) {
        perror(path);
        free(handle);
        return NULL;
    }
    return handle;
}

bool process_handle_is_valid(ProcessHandle* handle) {
    return ftell(handle->impl) >= 0;
}

void process_handle_free(ProcessHandle* handle) {
    fclose(handle->impl);
    free(handle);
}
