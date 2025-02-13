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

static bool process_handle_seek(ProcessHandle* handle, MemoryAddress addr) {
    if(ftell(handle->mem) != (int64_t)addr) {
        if(fseek(handle->mem, addr, SEEK_SET) != 0) {
            return false;
        }
    }
    return true;
}

size_t process_handle_read(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size) {
    if(!process_handle_seek(handle, addr)) {
        return 0;
    }
    uint64_t read = fread(buf, 1, size, handle->mem);
    if(read != size) {
        if(feof(handle->mem)) {
            // For some reason when reaching EOF, it sets error to 1 instead,
            // but perror() says "Input/output error" which should be 5 (EIO)
        } else if(ferror(handle->mem) != 1) {
            perror("Error reading process memory");
        }
    }
    return read;
}

size_t process_handle_write(ProcessHandle* handle, MemoryAddress addr, void* buf, size_t size) {
    if(!process_handle_seek(handle, addr)) {
        return 0;
    }
    uint64_t written = fwrite(buf, 1, size, handle->mem);
    if(written != size) {
        // FIXME: check this works correctly
        if(ferror(handle->mem)) {
            perror("Error writing process memory");
        }
    }
    return written;
}

void process_handle_free(ProcessHandle* handle) {
    fclose(handle->mem);
    free(handle);
}
