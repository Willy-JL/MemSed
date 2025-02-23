#include "handle.h"

#include <fcntl.h>

// Linux: file handle to /proc/pid/mem
struct ProcessHandle {
    ProcessPid pid;
    int32_t mem_fd;
};

ProcessHandle* process_handle_init(ProcessPid pid) {
    ProcessHandle* handle = malloc(sizeof(ProcessHandle));
    handle->pid = pid;
    char path[31];
    snprintf(path, sizeof(path), "/proc/%i/mem", pid);
    handle->mem_fd = open(path, O_RDWR);
    if(handle->mem_fd < 0) {
        perror(path);
        free(handle);
        return NULL;
    }
    return handle;
}

bool process_handle_is_valid(ProcessHandle* handle) {
    return lseek(handle->mem_fd, 0, SEEK_CUR) >= 0 && process_pid_is_alive(handle->pid);
}

size_t process_handle_read(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size) {
    ssize_t did_read = pread(handle->mem_fd, buf, size, addr);
    if(did_read < 0) {
        perror("Error reading process memory");
        return 0;
    }
    return did_read;
}

size_t process_handle_write(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size) {
    ssize_t did_write = pwrite(handle->mem_fd, buf, size, addr);
    if(did_write < 0) {
        perror("Error writing process memory");
        return 0;
    }
    return did_write;
}

void process_handle_free(ProcessHandle* handle) {
    close(handle->mem_fd);
    free(handle);
}
