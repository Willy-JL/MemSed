#include "search.h"
#include "../process/handle.h"
#include "../thread/thread.h"

const size_t chunk_size = 1024 * 1024;

struct MemorySearch {
    MemorySearchParams params;
    Process* process;
    ProcessHandle* handle;
    Thread* search_thread;
    flt32_t search_progress;
    MemorySearchResults results;
};

MemorySearch* memory_search_init() {
    MemorySearch* memory_search = malloc(sizeof(MemorySearch));
    memory_search->params.type = MemoryTypeInteger;
    memory_search->params.alignment = 4;
    memory_search->params.value = 0.0l;
    memory_search->params.precision = 0.1l;
    memory_search->process = NULL;
    memory_search->handle = NULL;
    memory_search->search_thread = NULL;
    memory_search->search_progress = 0.0f;
    memory_search->results.regions = NULL;
    memory_search->results.current_results_count = 0;
    memory_search->results.batches_count = 0;
    memory_search->results.batches = NULL;
    return memory_search;
}

void memory_search_process_attach(MemorySearch* memory_search, ProcessPid pid) {
    if(memory_search_process_is_attached(memory_search)) {
        return;
    }

    memory_search->process = process_init(pid);
    if(memory_search->process == NULL) {
        return;
    }
    memory_search->handle = process_handle_init(pid);
    if(memory_search->handle == NULL) {
        process_free(memory_search->process);
        memory_search->process = NULL;
        return;
    }

    // Example results for reference
    return;
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
    return memory_search->handle != NULL;
}

Process* memory_search_get_process(MemorySearch* memory_search) {
    return memory_search->process;
}

void memory_search_process_detach(MemorySearch* memory_search) {
    ProcessHandle* handle = memory_search->handle;
    Process* process = memory_search->process;
    memory_search->handle = NULL;
    memory_search->process = NULL;
    process_handle_free(handle);
    process_free(process);

    memory_search_reset(memory_search);
}

MemorySearchParams memory_search_get_params(MemorySearch* memory_search) {
    return memory_search->params;
}

void memory_search_set_params(MemorySearch* memory_search, MemorySearchParams params) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    if(memory_search->results.batches_count > 0) {
        // Can't change some values after first scan
        params.type = memory_search->params.type;
        params.alignment = memory_search->params.alignment;
    }

    params.type = CLAMP(params.type, MemoryTypeMAX - 1, 0);
    params.alignment = MAX(params.alignment, 1);
    if(params.type <= MemoryTypeInteger) {
        params.value = round(params.value);
    }
    if(params.type <= MemoryTypeUnsigned) {
        params.value = ABS(params.value);
    }
    params.precision = ABS(params.precision);

    memory_search->params = params;
}

static size_t memory_search_get_result_size(MemoryType type) {
    switch(memory_type_get_size(type)) {
    case 1:
        return sizeof(MemorySearchResult8);
    case 2:
        return sizeof(MemorySearchResult16);
    case 4:
        return sizeof(MemorySearchResult32);
    case 8:
        return sizeof(MemorySearchResult64);
    case 16:
        return sizeof(MemorySearchResult128);
    }
    unreachable();
}

static bool memory_search_should_process_type(MemoryType param, MemoryType type) {
    if(type >= MemoryTypeMAX) {
        return false;
    }
    if(type == MemoryTypeUnsigned || type == MemoryTypeSigned || type == MemoryTypeInteger ||
       type == MemoryTypeFloating || type == MemoryTypeNumber) {
        return false;
    }

    if(type == param) {
        return true;
    }

    switch(param) {
    case MemoryTypeUnsigned:
        return type < MemoryTypeUnsigned;
    case MemoryTypeSigned:
        return type < MemoryTypeSigned && type > MemoryTypeUnsigned;
    case MemoryTypeInteger:
        return type < MemoryTypeInteger;
    case MemoryTypeFloating:
        return type < MemoryTypeFloating && type > MemoryTypeInteger;
    case MemoryTypeNumber:
        return type < MemoryTypeNumber;
    default:
        unreachable();
    }
}

static void* memory_search_begin_callback(void* context) {
    MemorySearch* memory_search = context;

    ProcessRegions* regions = process_regions_init(memory_search->process->pid);
    if(regions == NULL) {
        return NULL;
    }

    MemorySearchResultBatch* batch = malloc(sizeof(MemorySearchResultBatch) * 1);
    batch->total_results_count = 0;
    batch->sets_count = 0;
    batch->sets = malloc(sizeof(MemorySearchResultSet) * MemoryTypeMAX);
    size_t capacities[MemoryTypeMAX];
    size_t max_type_size = 0;

    for(MemoryType type = 0; type < MemoryTypeMAX; type++) {
        if(!memory_search_should_process_type(memory_search->params.type, type)) {
            continue;
        }
        MemorySearchResultSet* set = &batch->sets[batch->sets_count];
        set->type = type;
        set->results_count = 0;
        capacities[batch->sets_count] = 1;
        set->results = malloc(memory_search_get_result_size(type) * capacities[batch->sets_count]);
        max_type_size = MAX(max_type_size, memory_type_get_size(type));
        batch->sets_count++;
    }
    batch->sets = realloc(batch->sets, sizeof(MemorySearchResultSet) * batch->sets_count);

    void* chunk_buf = malloc(chunk_size);
    ProcessHandle* handle = memory_search->handle;
    uint8_t alignment = memory_search->params.alignment;
    for(size_t region_i = 0; region_i < regions->regions_count; region_i++) {
        ProcessRegion* region = &regions->regions[region_i];
        MemoryAddress addr = region->start;
        MemoryAddress chunk_addr = 0;
        size_t chunk_len = 0;
        void* chunk_cur;
        while(addr < region->end) {
            // FIXME: show progress
            if(addr + max_type_size > chunk_addr + chunk_len) {
                chunk_addr = addr;
                chunk_len = process_handle_read(handle, chunk_addr, chunk_buf, chunk_size);
                if(chunk_len == 0) {
                    break;
                }
                chunk_cur = chunk_buf;
            }
            size_t chunk_avail = chunk_len - (addr - chunk_addr);

            // FIXME: check values and save results
            UNUSED(chunk_cur);
            UNUSED(chunk_avail);

            addr += alignment;
            chunk_cur += alignment;
        }
    }
    free(chunk_buf);

    for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
        MemorySearchResultSet* set = &batch->sets[set_i];
        batch->total_results_count += set->results_count;
        if(set->results_count > 0) {
            set->results = realloc(
                set->results,
                memory_search_get_result_size(set->type) * set->results_count);
        }
    }

    memory_search->results.batches = batch;
    memory_search->results.batches_count = 1;
    memory_search->results.current_results_count = batch->total_results_count;
    memory_search->results.regions = regions;

    return NULL;
}

static void* memory_search_next_callback(void* context) {
    MemorySearch* memory_search = context;
    UNUSED(memory_search);
    return NULL;
}

void memory_search_begin(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    memory_search->search_thread = thread_start(memory_search_begin_callback, memory_search);
}

void memory_search_next(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    memory_search->search_thread = thread_start(memory_search_next_callback, memory_search);
}

bool memory_search_is_searching(MemorySearch* memory_search) {
    return memory_search->search_thread != NULL;
}

flt32_t memory_search_get_search_progress(MemorySearch* memory_search) {
    return memory_search->search_progress;
}

void memory_search_stop(MemorySearch* memory_search) {
    if(!memory_search_is_searching(memory_search)) {
        return;
    }
    thread_stop(memory_search->search_thread);
}

void memory_search_undo(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    if(memory_search->results.batches_count < 2) {
        return;
    }

    memory_search->results.batches_count--;
    memory_search->results.current_results_count =
        memory_search->results.batches[memory_search->results.batches_count - 1]
            .total_results_count;
    memory_search->results.batches = realloc(
        memory_search->results.batches,
        sizeof(MemorySearchResultBatch) * memory_search->results.batches_count);
}

void memory_search_reset(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }

    if(memory_search->results.regions != NULL) {
        process_regions_free(memory_search->results.regions);
        memory_search->results.regions = NULL;
    }

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

MemorySearchResults* memory_search_get_results(MemorySearch* memory_search) {
    return &memory_search->results;
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

void memory_search_tick(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        if(thread_tryjoin(memory_search->search_thread, NULL)) {
            memory_search->search_thread = NULL;
        }
    } else if(memory_search_process_is_attached(memory_search)) {
        if(!process_handle_is_valid(memory_search->handle)) {
            memory_search_process_detach(memory_search);
        }
        // FIXME: periodically update latest result values
    }
}

void memory_search_free(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        memory_search_stop(memory_search);
        thread_join(memory_search->search_thread, NULL);
    }
    if(memory_search_process_is_attached(memory_search)) {
        memory_search_process_detach(memory_search);
    }
    free(memory_search);
}
