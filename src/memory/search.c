#include "search.h"

struct MemorySearch {
    MemorySearchParams params;
    MemoryPid pid;
    bool is_searching;
    flt32_t search_progress;
    MemorySearchResults results;
};

MemorySearch* memory_search_init() {
    MemorySearch* memory_search = malloc(sizeof(MemorySearch));
    memory_search->params.type = MemoryTypeInteger;
    memory_search->params.alignment = 4;
    memory_search->params.value = 0.0l;
    memory_search->params.precision = 0.1l;
    memory_search->pid = 0;
    memory_search->is_searching = false;
    memory_search->search_progress = 0.0f;
    memory_search->results.current_results_count = 0;
    memory_search->results.batches_count = 0;
    memory_search->results.batches = NULL;
    return memory_search;
}

void memory_search_process_attach(MemorySearch* memory_search, MemoryPid pid) {
    // FIXME: does this need to do anything else, or all done when searching?
    memory_search->pid = pid;

    // Example results for testing
    memory_search->results.batches = malloc(sizeof(MemorySearchResultBatch) * 2);
    memory_search->results.batches[0].sets = malloc(sizeof(MemorySearchResultSet) * 2);
    memory_search->results.batches[0].sets[0].type = MemoryTypeI8;
    memory_search->results.batches[0].sets[0].results_8 = malloc(sizeof(MemorySearchResult8) * 2);
    memory_search->results.batches[0].sets[0].results_8[0].address = 0x123456781234;
    memory_search->results.batches[0].sets[0].results_8[0].i8 = 69;
    memory_search->results.batches[0].sets[0].results_8[1].address = 0x876543214321;
    memory_search->results.batches[0].sets[0].results_8[1].i8 = 42;
    memory_search->results.batches[0].sets[0].results_count = 2;
    memory_search->results.batches[0].sets[1].type = MemoryTypeF64;
    memory_search->results.batches[0].sets[1].results_64 =
        malloc(sizeof(MemorySearchResult64) * 1);
    memory_search->results.batches[0].sets[1].results_64[0].address = 0x112233445566;
    memory_search->results.batches[0].sets[1].results_64[0].f64 = 420.69;
    memory_search->results.batches[0].sets[1].results_count = 1;
    memory_search->results.batches[0].sets_count = 2;
    memory_search->results.batches[0].total_results_count = 3;
    memory_search->results.batches[1].sets = malloc(sizeof(MemorySearchResultSet) * 2);
    memory_search->results.batches[1].sets[0].type = MemoryTypeI8;
    memory_search->results.batches[1].sets[0].results_8 = malloc(sizeof(MemorySearchResult8) * 1);
    memory_search->results.batches[1].sets[0].results_8[0].address = 0x123456781234;
    memory_search->results.batches[1].sets[0].results_8[0].i8 = -42;
    memory_search->results.batches[1].sets[0].results_count = 1;
    memory_search->results.batches[1].sets[1].type = MemoryTypeF64;
    memory_search->results.batches[1].sets[1].results_64 =
        malloc(sizeof(MemorySearchResult64) * 1);
    memory_search->results.batches[1].sets[1].results_64[0].address = 0x112233445566;
    memory_search->results.batches[1].sets[1].results_64[0].f64 = -69.4200000000;
    memory_search->results.batches[1].sets[1].results_count = 1;
    memory_search->results.batches[1].sets_count = 2;
    memory_search->results.batches[1].total_results_count = 2;
    memory_search->results.batches_count = 2;
    memory_search->results.current_results_count = 2;
}

bool memory_search_process_is_attached(MemorySearch* memory_search) {
    // FIXME: periodically check if process is still alive
    return memory_search->pid != 0;
}

void memory_search_process_detach(MemorySearch* memory_search) {
    memory_search->pid = 0;
    size_t batches_count = memory_search->results.batches_count;
    MemorySearchResultBatch* batches = memory_search->results.batches;
    memory_search->results.current_results_count = 0;
    memory_search->results.batches_count = 0;
    memory_search->results.batches = NULL;
    if(batches_count >= 1) {
        for(size_t batch_i = 0; batch_i < batches_count; batch_i++) {
            MemorySearchResultBatch* batch = &batches[batch_i];
            size_t sets_count = batch->sets_count;
            MemorySearchResultSet* sets = batch->sets;
            batch->total_results_count = 0;
            batch->sets_count = 0;
            batch->sets = NULL;
            for(size_t set_i = 0; set_i < sets_count; set_i++) {
                MemorySearchResultSet* set = &sets[set_i];
                set->results_count = 0;
                free(set->results);
            }
            free(sets);
        }
        free(batches);
    }
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

bool memory_search_is_searching(MemorySearch* memory_search) {
    return memory_search->is_searching;
}

flt32_t memory_search_get_search_progress(MemorySearch* memory_search) {
    return memory_search->search_progress;
}

MemorySearchResults memory_search_get_results(MemorySearch* memory_search) {
    return memory_search->results;
}

MemoryAddress memory_search_get_result_address(MemorySearchResultSet* set, size_t i) {
    switch(set->type) {
    case MemoryTypeUnsigned:
    case MemoryTypeSigned:
    case MemoryTypeInteger:
    case MemoryTypeFloating:
    case MemoryTypeNumber:
    case MemoryTypeMAX:
        unreachable();

    case MemoryTypeU8: {
        MemorySearchResult8* result = &set->results_8[i];
        return result->address;
        break;
    }
    case MemoryTypeU16: {
        MemorySearchResult16* result = &set->results_16[i];
        return result->address;
        break;
    }
    case MemoryTypeU32: {
        MemorySearchResult32* result = &set->results_32[i];
        return result->address;
        break;
    }
    case MemoryTypeU64: {
        MemorySearchResult64* result = &set->results_64[i];
        return result->address;
        break;
    }
    case MemoryTypeI8: {
        MemorySearchResult8* result = &set->results_8[i];
        return result->address;
        break;
    }
    case MemoryTypeI16: {
        MemorySearchResult16* result = &set->results_16[i];
        return result->address;
        break;
    }
    case MemoryTypeI32: {
        MemorySearchResult32* result = &set->results_32[i];
        return result->address;
        break;
    }
    case MemoryTypeI64: {
        MemorySearchResult64* result = &set->results_64[i];
        return result->address;
        break;
    }
    case MemoryTypeF32: {
        MemorySearchResult32* result = &set->results_32[i];
        return result->address;
        break;
    }
    case MemoryTypeF64: {
        MemorySearchResult64* result = &set->results_64[i];
        return result->address;
        break;
    }
    case MemoryTypeF128: {
        MemorySearchResult128* result = &set->results_128[i];
        return result->address;
        break;
    }
    }
}

MemorySearchResultDisplay memory_search_get_result_display(MemorySearchResultSet* set, size_t i) {
    MemorySearchResultDisplay display;
    MemoryAddress address;

    switch(set->type) {
    case MemoryTypeUnsigned:
    case MemoryTypeSigned:
    case MemoryTypeInteger:
    case MemoryTypeFloating:
    case MemoryTypeNumber:
    case MemoryTypeMAX:
        unreachable();

    case MemoryTypeU8: {
        MemorySearchResult8* result = &set->results_8[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%hhu", result->u8);
        break;
    }
    case MemoryTypeU16: {
        MemorySearchResult16* result = &set->results_16[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%hu", result->u16);
        break;
    }
    case MemoryTypeU32: {
        MemorySearchResult32* result = &set->results_32[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%u", result->u32);
        break;
    }
    case MemoryTypeU64: {
        MemorySearchResult64* result = &set->results_64[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%lu", result->u64);
        break;
    }
    case MemoryTypeI8: {
        MemorySearchResult8* result = &set->results_8[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%hhi", result->i8);
        break;
    }
    case MemoryTypeI16: {
        MemorySearchResult16* result = &set->results_16[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%hi", result->i16);
        break;
    }
    case MemoryTypeI32: {
        MemorySearchResult32* result = &set->results_32[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%i", result->i32);
        break;
    }
    case MemoryTypeI64: {
        MemorySearchResult64* result = &set->results_64[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%li", result->i64);
        break;
    }
    case MemoryTypeF32: {
        MemorySearchResult32* result = &set->results_32[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%.*g", FLT_DIG, result->f32);
        break;
    }
    case MemoryTypeF64: {
        MemorySearchResult64* result = &set->results_64[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%.*lg", DBL_DIG, result->f64);
        break;
    }
    case MemoryTypeF128: {
        MemorySearchResult128* result = &set->results_128[i];
        address = result->address;
        snprintf(display.value_str, sizeof(display.value_str), "%.*Lg", LDBL_DIG, result->f128);
        break;
    }
    }

    snprintf(display.address_str, sizeof(display.address_str), "0x%" PRIXPTR, (uintptr_t)address);

    return display;
}

void memory_search_free(MemorySearch* memory_search) {
    // FIXME: cleanup
    free(memory_search);
}
