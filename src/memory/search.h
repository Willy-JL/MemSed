#pragma once

#include "pid.h"
#include "type.h"

typedef struct {
    MemoryType type;
    uint8_t alignment;
    flt128_t value;
    flt128_t precision;
} MemorySearchParams;

typedef struct MemorySearch MemorySearch;

MemorySearch* memory_search_init();
void memory_search_process_attach(MemorySearch* memory_search, MemoryPid pid);
bool memory_search_process_is_attached(MemorySearch* memory_search);
void memory_search_process_detach(MemorySearch* memory_search);
MemorySearchParams memory_search_get_params(MemorySearch* memory_search);
void memory_search_set_params(MemorySearch* memory_search, MemorySearchParams params);
void memory_search_free(MemorySearch* memory_search);
