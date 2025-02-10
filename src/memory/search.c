#include "search.h"

struct MemorySearch {
    MemorySearchParams params;
    MemoryPid pid;
};

MemorySearch* memory_search_init() {
    MemorySearch* memory_search = malloc(sizeof(MemorySearch));
    memory_search->params.type = MemoryTypeInteger;
    memory_search->params.alignment = 4;
    memory_search->params.value = 0.0l;
    memory_search->params.precision = 0.1l;
    memory_search->pid = 0;
    return memory_search;
}

void memory_search_process_attach(MemorySearch* memory_search, MemoryPid pid) {
    memory_search->pid = pid;
    // FIXME: does this need to do anything else, or all done when searching?
}

bool memory_search_process_is_attached(MemorySearch* memory_search) {
    // FIXME: periodically check if process is still alive
    return memory_search->pid != 0;
}

void memory_search_process_detach(MemorySearch* memory_search) {
    memory_search->pid = 0;
    // FIXME: cleanup
}

MemorySearchParams memory_search_get_params(MemorySearch* memory_search) {
    return memory_search->params;
}

void memory_search_set_params(MemorySearch* memory_search, MemorySearchParams params) {
    // FIXME: reject if already searching
    if(params.type <= MemoryTypeInteger) {
        params.value = round(params.value);
    }
    if(params.type <= MemoryTypeUnsigned) {
        params.value = ABS(params.value);
    }
    memory_search->params = params;
}

void memory_search_free(MemorySearch* memory_search) {
    // FIXME: cleanup
    free(memory_search);
}
