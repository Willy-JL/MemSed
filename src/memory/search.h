#pragma once

#include "../process/process.h"
#include "../process/regions.h"
#include "address.h"
#include "type.h"

typedef struct {
    char address_str[19]; // (64bit = 8byte) * 2hexchars + "0x" + '\0'
    char value_str[33]; // i64 = 20digit, floats could be more but 32chars are good enough
} MemorySearchResultDisplay;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    union {
        uint8_t u8;
        int8_t i8;
    };
} MemorySearchResult8;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    union {
        uint16_t u16;
        int16_t i16;
    };
} MemorySearchResult16;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    union {
        uint32_t u32;
        int32_t i32;
        flt32_t f32;
    };
} MemorySearchResult32;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    union {
        uint64_t u64;
        int64_t i64;
        flt64_t f64;
    };
} MemorySearchResult64;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    union {
        flt128_t f128;
    };
} MemorySearchResult128;

typedef struct {
    MemoryType type;
    size_t results_count;
    union {
        void* results;
        MemorySearchResult8* results_8;
        MemorySearchResult16* results_16;
        MemorySearchResult32* results_32;
        MemorySearchResult64* results_64;
        MemorySearchResult128* results_128;
    };
} MemorySearchResultSet;

typedef struct {
    size_t total_results_count;
    size_t sets_count;
    MemorySearchResultSet* sets;
} MemorySearchResultBatch;

typedef struct {
    ProcessRegions* regions;
    size_t current_results_count;
    size_t batches_count;
    MemorySearchResultBatch* batches;
} MemorySearchResults;

typedef struct {
    MemoryType type;
    uint8_t alignment;
    flt128_t value;
    flt128_t precision;
} MemorySearchParams;

typedef struct __attribute__((packed)) {
    MemoryAddress address;
    MemoryType type;
    bool active;
    char description[67];
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        flt32_t f32;
        flt64_t f64;
        flt128_t f128;
    } value;
} MemorySearchScratchpadItem;

typedef struct {
    size_t items_count;
    MemorySearchScratchpadItem* items;
} MemorySearchScratchpad;

typedef struct MemorySearch MemorySearch;

MemorySearch* memory_search_init();
void memory_search_process_attach(MemorySearch* memory_search, ProcessPid pid);
bool memory_search_process_is_attached(MemorySearch* memory_search);
Process* memory_search_get_process(MemorySearch* memory_search);
void memory_search_process_detach(MemorySearch* memory_search);
MemorySearchParams memory_search_get_params(MemorySearch* memory_search);
void memory_search_set_params(MemorySearch* memory_search, MemorySearchParams params);
void memory_search_begin(MemorySearch* memory_search);
void memory_search_next(MemorySearch* memory_search);
bool memory_search_is_searching(MemorySearch* memory_search);
flt32_t memory_search_get_search_progress(MemorySearch* memory_search);
void memory_search_stop(MemorySearch* memory_search);
void memory_search_undo(MemorySearch* memory_search);
void memory_search_reset(MemorySearch* memory_search);
MemorySearchResults* memory_search_get_results(MemorySearch* memory_search);
MemoryAddress memory_search_get_result_address(MemorySearchResultSet* set, size_t i);
MemorySearchResultDisplay memory_search_get_result_display(MemorySearchResultSet* set, size_t i);
void memory_search_scratchpad_add(MemorySearch* memory_search, MemoryAddress addr, MemoryType type);
MemorySearchScratchpad* memory_search_get_scratchpad(MemorySearch* memory_search);
MemorySearchResultDisplay memory_search_get_scratchpad_display(MemorySearchScratchpadItem* item);
void memory_search_scratchpad_del(MemorySearch* memory_search, MemorySearchScratchpadItem* item);
void memory_search_tick(MemorySearch* memory_search);
void memory_search_free(MemorySearch* memory_search);
