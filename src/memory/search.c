#include "search.h"
#include "../process/handle.h"
#include "../thread/thread.h"

const size_t chunk_size = 1024 * 1024;
const size_t update_delay_usec = 20;
const size_t memory_search_update_max_results = 100'000;

struct MemorySearch {
    MemorySearchParams params;
    Process* process;
    ProcessHandle* handle;
    Thread* update_thread;
    Thread* search_thread;
    flt32_t search_progress;
    MemorySearchResults results;
    MemorySearchScratchpad scratchpad;
};

MemorySearch* memory_search_init() {
    MemorySearch* memory_search = malloc(sizeof(MemorySearch));
    memory_search->params.type = MemoryTypeNumber;
    memory_search->params.alignment = 4;
    memory_search->params.region_types = ProcessRegionTypeHeap | ProcessRegionTypeStack |
                                         ProcessRegionTypeAnonymous;
    memory_search->params.region_flags = ProcessRegionFlagRead | ProcessRegionFlagWrite;
    memory_search->params.value = 123.0l;
    memory_search->params.deviation = 0.1l;
    memory_search->process = NULL;
    memory_search->handle = NULL;
    memory_search->update_thread = NULL;
    memory_search->search_thread = NULL;
    memory_search->search_progress = 0.0f;
    memory_search->results.regions = NULL;
    memory_search->results.current_results_count = 0;
    memory_search->results.batches_count = 0;
    memory_search->results.batches = NULL;
    memory_search->scratchpad.items_count = 0;
    memory_search->scratchpad.items = NULL;
    return memory_search;
}

static void memory_search_stop_update(MemorySearch* memory_search) {
    if(memory_search->update_thread != NULL) {
        thread_cancel(memory_search->update_thread);
        thread_join(memory_search->update_thread, NULL);
        memory_search->update_thread = NULL;
    }
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
}

bool memory_search_process_is_attached(MemorySearch* memory_search) {
    return memory_search->handle != NULL;
}

Process* memory_search_get_process(MemorySearch* memory_search) {
    return memory_search->process;
}

void memory_search_process_detach(MemorySearch* memory_search) {
    memory_search_stop_update(memory_search);
    ProcessHandle* handle = memory_search->handle;
    Process* process = memory_search->process;
    memory_search->handle = NULL;
    memory_search->process = NULL;
    process_handle_free(handle);
    process_free(process);

    memory_search_reset(memory_search);
    memory_search_scratchpad_wipe(memory_search);
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
        params.region_types = memory_search->params.region_types;
        params.region_flags = memory_search->params.region_flags;
    }

    params.type = CLAMP(params.type, MemoryTypeMAX - 1, 0);
    params.alignment = MAX(params.alignment, 1);
    params.deviation = ABS(params.deviation);
    if(params.type <= MemoryTypeInteger) {
        params.value = round(params.value);
        params.deviation = round(params.deviation);
    }
    if(params.type <= MemoryTypeUnsigned) {
        params.value = ABS(params.value);
    }

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

static bool memory_search_should_process_type(MemoryType param, flt128_t value, MemoryType type) {
    if(type >= MemoryTypeMAX) {
        return false;
    }
    if(type == MemoryTypeUnsigned || type == MemoryTypeSigned || type == MemoryTypeInteger ||
       type == MemoryTypeFloating || type == MemoryTypeNumber) {
        return false;
    }

    if(type == param) {
        switch(type) {
        case MemoryTypeU8:
            return value >= 0 && value <= UINT8_MAX;
        case MemoryTypeU16:
            return value >= 0 && value <= UINT16_MAX;
        case MemoryTypeU32:
            return value >= 0 && value <= UINT32_MAX;
        case MemoryTypeU64:
            return value >= 0 && value <= UINT64_MAX;
        case MemoryTypeI8:
            return value >= INT8_MIN && value <= INT8_MAX;
        case MemoryTypeI16:
            return value >= INT16_MIN && value <= INT16_MAX;
        case MemoryTypeI32:
            return value >= INT32_MIN && value <= INT32_MAX;
        case MemoryTypeI64:
            return value >= INT64_MIN && value <= INT64_MAX;
        case MemoryTypeF32:
            return value >= FLT_MIN && value <= FLT_MAX;
        case MemoryTypeF64:
            return value >= DBL_MIN && value <= DBL_MAX;
        case MemoryTypeF128:
            return value >= LDBL_MIN && value <= LDBL_MAX;
        default:
            unreachable();
        }
    }

    switch(param) {
    case MemoryTypeUnsigned:
        if(type < MemoryTypeUnsigned) {
            return memory_search_should_process_type(type, value, type);
        }
        return false;
    case MemoryTypeSigned:
        if(type < MemoryTypeSigned && type > MemoryTypeUnsigned) {
            return memory_search_should_process_type(type, value, type);
        }
        return false;
    case MemoryTypeInteger:
        if(type < MemoryTypeInteger) {
            return memory_search_should_process_type(type, value, type);
        }
        return false;
    case MemoryTypeFloating:
        if(type < MemoryTypeFloating && type > MemoryTypeInteger) {
            return memory_search_should_process_type(type, value, type);
        }
        return false;
    case MemoryTypeNumber:
        if(type < MemoryTypeNumber) {
            return memory_search_should_process_type(type, value, type);
        }
        return false;
    default:
        return false;
    }
}

static void memory_search_consolidate_results(void* context) {
    MemorySearch* memory_search = context;
    MemorySearchResultBatch* batch =
        &memory_search->results.batches[memory_search->results.batches_count - 1];
    batch->total_results_count = 0;
    for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
        MemorySearchResultSet* set = &batch->sets[set_i];
        batch->total_results_count += set->results_count;
        if(set->results_count > 0) {
            set->results = realloc(
                set->results,
                memory_search_get_result_size(set->type) * set->results_count);
        }
    }
    memory_search->results.current_results_count = batch->total_results_count;
}

static void memory_search_extend_results(MemorySearchResultSet* set, size_t* capacity) {
    if(set->results_count == *capacity) {
        *capacity *= 2;
        set->results = realloc(set->results, memory_search_get_result_size(set->type) * *capacity);
    }
}

static void* memory_search_begin_callback(void* context) {
    MemorySearch* memory_search = context;
    memory_search_stop_update(memory_search);

    ProcessRegions* regions = process_regions_init(
        memory_search->process->pid,
        memory_search->params.region_types,
        memory_search->params.region_flags);
    if(regions == NULL) {
        return NULL;
    }

    MemorySearchResultBatch* batch = malloc(sizeof(MemorySearchResultBatch) * 1);
    batch->sets_count = 0;
    batch->sets = malloc(sizeof(MemorySearchResultSet) * MemoryTypeMAX);
    size_t capacities[MemoryTypeMAX];
    size_t max_type_size = 0;

    flt128_t value = memory_search->params.value;
    flt128_t deviation = memory_search->params.deviation;
    uint8_t value_u8_min = value - deviation;
    uint8_t value_u8_max = value + deviation;
    uint16_t value_u16_min = value - deviation;
    uint16_t value_u16_max = value + deviation;
    uint32_t value_u32_min = value - deviation;
    uint32_t value_u32_max = value + deviation;
    uint64_t value_u64_min = value - deviation;
    uint64_t value_u64_max = value + deviation;
    int8_t value_i8_min = value - deviation;
    int8_t value_i8_max = value + deviation;
    int16_t value_i16_min = value - deviation;
    int16_t value_i16_max = value + deviation;
    int32_t value_i32_min = value - deviation;
    int32_t value_i32_max = value + deviation;
    int64_t value_i64_min = value - deviation;
    int64_t value_i64_max = value + deviation;
    flt32_t value_f32_min = value - deviation;
    flt32_t value_f32_max = value + deviation;
    flt64_t value_f64_min = value - deviation;
    flt64_t value_f64_max = value + deviation;
    flt128_t value_f128_min = value - deviation;
    flt128_t value_f128_max = value + deviation;

    for(MemoryType type = MemoryTypeMAX - 1; type < MemoryTypeMAX; type--) {
        if(!memory_search_should_process_type(memory_search->params.type, value, type)) {
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

    memory_search->results.batches = batch;
    memory_search->results.batches_count++;
    memory_search->results.regions = regions;
    thread_self_push_cancel_cleanup(memory_search_consolidate_results, memory_search);

    void* chunk_buf = malloc(chunk_size);
    thread_self_push_cancel_cleanup(free, chunk_buf);
    ProcessHandle* handle = memory_search->handle;
    uint8_t alignment = memory_search->params.alignment;
    size_t regions_progress = 0;
    for(size_t region_i = 0; region_i < regions->regions_count; region_i++) {
        ProcessRegion* region = &regions->regions[region_i];
        MemoryAddress addr = region->start;
        MemoryAddress chunk_addr = 0;
        MemoryAddress chunk_end = 0;
        MemoryAddress chunk_end_max_type_margin = 0;
        void* chunk_cur;
        while(addr < region->end) {
            if(addr > chunk_end_max_type_margin) {
                thread_self_quit_if_canceled();
                memory_search->search_progress =
                    (flt32_t)(regions_progress + (addr - region->start)) / regions->total_size;
                chunk_addr = addr;
                size_t chunk_len = process_handle_read(handle, chunk_addr, chunk_buf, chunk_size);
                if(chunk_len == 0) {
                    break;
                }
                chunk_end = chunk_addr + chunk_len;
                chunk_end_max_type_margin = chunk_end - max_type_size;
                chunk_cur = chunk_buf;
            }

            for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
                MemorySearchResultSet* set = &batch->sets[set_i];
                switch(set->type) {
                case MemoryTypeU8:
                    if(*(uint8_t*)chunk_cur < value_u8_min ||
                       *(uint8_t*)chunk_cur > value_u8_max) {
                        continue;
                    }
                    if(chunk_end - addr < 1) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_8[set->results_count].base.address = addr;
                    set->results_8[set->results_count].base.prev_i = -1;
                    set->results_8[set->results_count].u8 = *(uint8_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeU16:
                    if(*(uint16_t*)chunk_cur < value_u16_min ||
                       *(uint16_t*)chunk_cur > value_u16_max) {
                        continue;
                    }
                    if(chunk_end - addr < 2) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_16[set->results_count].base.address = addr;
                    set->results_16[set->results_count].base.prev_i = -1;
                    set->results_16[set->results_count].u16 = *(uint16_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeU32:
                    if(*(uint32_t*)chunk_cur < value_u32_min ||
                       *(uint32_t*)chunk_cur > value_u32_max) {
                        continue;
                    }
                    if(chunk_end - addr < 4) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_32[set->results_count].base.address = addr;
                    set->results_32[set->results_count].base.prev_i = -1;
                    set->results_32[set->results_count].u32 = *(uint32_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeU64:
                    if(*(uint64_t*)chunk_cur < value_u64_min ||
                       *(uint64_t*)chunk_cur > value_u64_max) {
                        continue;
                    }
                    if(chunk_end - addr < 8) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_64[set->results_count].base.address = addr;
                    set->results_64[set->results_count].base.prev_i = -1;
                    set->results_64[set->results_count].u64 = *(uint64_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeI8:
                    if(*(int8_t*)chunk_cur < value_i8_min || *(int8_t*)chunk_cur > value_i8_max) {
                        continue;
                    }
                    if(chunk_end - addr < 1) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_8[set->results_count].base.address = addr;
                    set->results_8[set->results_count].base.prev_i = -1;
                    set->results_8[set->results_count].i8 = *(int8_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeI16:
                    if(*(int16_t*)chunk_cur < value_i16_min ||
                       *(int16_t*)chunk_cur > value_i16_max) {
                        continue;
                    }
                    if(chunk_end - addr < 2) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_16[set->results_count].base.address = addr;
                    set->results_16[set->results_count].base.prev_i = -1;
                    set->results_16[set->results_count].i16 = *(int16_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeI32:
                    if(*(int32_t*)chunk_cur < value_i32_min ||
                       *(int32_t*)chunk_cur > value_i32_max) {
                        continue;
                    }
                    if(chunk_end - addr < 4) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_32[set->results_count].base.address = addr;
                    set->results_32[set->results_count].base.prev_i = -1;
                    set->results_32[set->results_count].i32 = *(int32_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeI64:
                    if(*(int64_t*)chunk_cur < value_i64_min ||
                       *(int64_t*)chunk_cur > value_i64_max) {
                        continue;
                    }
                    if(chunk_end - addr < 8) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_64[set->results_count].base.address = addr;
                    set->results_64[set->results_count].base.prev_i = -1;
                    set->results_64[set->results_count].i64 = *(int64_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeF32:
                    if(isnanf(*(flt32_t*)chunk_cur) || *(flt32_t*)chunk_cur < (value_f32_min) ||
                       *(flt32_t*)chunk_cur > (value_f32_max)) {
                        continue;
                    }
                    if(chunk_end - addr < 4) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_32[set->results_count].base.address = addr;
                    set->results_32[set->results_count].base.prev_i = -1;
                    set->results_32[set->results_count].f32 = *(flt32_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeF64:
                    if(isnan(*(flt64_t*)chunk_cur) || *(flt64_t*)chunk_cur < (value_f64_min) ||
                       *(flt64_t*)chunk_cur > (value_f64_max)) {
                        continue;
                    }
                    if(chunk_end - addr < 8) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_64[set->results_count].base.address = addr;
                    set->results_64[set->results_count].base.prev_i = -1;
                    set->results_64[set->results_count].f64 = *(flt64_t*)chunk_cur;
                    set->results_count++;
                    break;
                case MemoryTypeF128:
                    if(isnanl(*(flt128_t*)chunk_cur) || *(flt128_t*)chunk_cur < (value_f128_min) ||
                       *(flt128_t*)chunk_cur > (value_f128_max)) {
                        continue;
                    }
                    if(chunk_end - addr < 16) {
                        continue;
                    }
                    memory_search_extend_results(set, &capacities[set_i]);
                    set->results_128[set->results_count].base.address = addr;
                    set->results_128[set->results_count].base.prev_i = -1;
                    set->results_128[set->results_count].f128 = *(flt128_t*)chunk_cur;
                    set->results_count++;
                    break;
                default:
                    unreachable();
                }
            }

            addr += alignment;
            chunk_cur += alignment;
        }
        regions_progress += region->end - region->start;
    }
    thread_self_pop_cancel_cleanup(true); // free(chunk_buf)

    thread_self_pop_cancel_cleanup(true); // memory_search_consolidate_results(memory_search)
    return NULL;
}

static void* memory_search_next_callback(void* context) {
    MemorySearch* memory_search = context;
    memory_search_stop_update(memory_search);

    memory_search->results.batches = realloc(
        memory_search->results.batches,
        sizeof(MemorySearchResultBatch) * (memory_search->results.batches_count + 1));
    MemorySearchResultBatch* last_batch =
        &memory_search->results.batches[memory_search->results.batches_count - 1];
    MemorySearchResultBatch* batch =
        &memory_search->results.batches[memory_search->results.batches_count];
    batch->sets_count = 0;
    batch->sets = malloc(sizeof(MemorySearchResultSet) * last_batch->sets_count);
    size_t capacities[MemoryTypeMAX];
    size_t max_type_size = 0;

    flt128_t value = memory_search->params.value;
    flt128_t deviation = memory_search->params.deviation;
    uint8_t value_u8_min = value - deviation;
    uint8_t value_u8_max = value + deviation;
    uint16_t value_u16_min = value - deviation;
    uint16_t value_u16_max = value + deviation;
    uint32_t value_u32_min = value - deviation;
    uint32_t value_u32_max = value + deviation;
    uint64_t value_u64_min = value - deviation;
    uint64_t value_u64_max = value + deviation;
    int8_t value_i8_min = value - deviation;
    int8_t value_i8_max = value + deviation;
    int16_t value_i16_min = value - deviation;
    int16_t value_i16_max = value + deviation;
    int32_t value_i32_min = value - deviation;
    int32_t value_i32_max = value + deviation;
    int64_t value_i64_min = value - deviation;
    int64_t value_i64_max = value + deviation;
    flt32_t value_f32_min = value - deviation;
    flt32_t value_f32_max = value + deviation;
    flt64_t value_f64_min = value - deviation;
    flt64_t value_f64_max = value + deviation;
    flt128_t value_f128_min = value - deviation;
    flt128_t value_f128_max = value + deviation;

    for(size_t last_set_i = 0; last_set_i < last_batch->sets_count; last_set_i++) {
        MemorySearchResultSet* last_set = &last_batch->sets[last_set_i];
        if(last_set->results_count == 0) {
            continue;
        }
        MemorySearchResultSet* set = &batch->sets[batch->sets_count];
        MemoryType type = last_set->type;
        set->type = type;
        set->results_count = 0;
        capacities[batch->sets_count] = 1;
        set->results = malloc(memory_search_get_result_size(type) * capacities[batch->sets_count]);
        max_type_size = MAX(max_type_size, memory_type_get_size(type));
        batch->sets_count++;
    }
    batch->sets = realloc(batch->sets, sizeof(MemorySearchResultSet) * batch->sets_count);

    memory_search->results.batches_count++;
    thread_self_push_cancel_cleanup(memory_search_consolidate_results, memory_search);

    void* value_buf = malloc(max_type_size);
    thread_self_push_cancel_cleanup(free, value_buf);
    ProcessHandle* handle = memory_search->handle;
    size_t results_progress = 0;
    size_t set_i = -1;
    for(size_t last_set_i = 0; last_set_i < last_batch->sets_count; last_set_i++) {
        MemorySearchResultSet* last_set = &last_batch->sets[last_set_i];
        if(last_set->results_count == 0) {
            continue;
        }
        MemorySearchResultSet* set = &batch->sets[++set_i];
        for(size_t result_i = 0; result_i < last_set->results_count; result_i++) {
            // FIXME: check if these are slowing down the search and make it faster
            thread_self_quit_if_canceled();
            memory_search->search_progress =
                (flt32_t)(results_progress + result_i) / last_batch->total_results_count;

            MemoryAddress addr;
            switch(set->type) {
            case MemoryTypeU8:
                addr = last_set->results_8[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 1) != 1) {
                    continue;
                }
                if(*(uint8_t*)value_buf < value_u8_min || *(uint8_t*)value_buf > value_u8_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_8[set->results_count].base.address = addr;
                set->results_8[set->results_count].base.prev_i = result_i;
                set->results_8[set->results_count].u8 = *(uint8_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeU16:
                addr = last_set->results_16[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 2) != 2) {
                    continue;
                }
                if(*(uint16_t*)value_buf < value_u16_min ||
                   *(uint16_t*)value_buf > value_u16_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_16[set->results_count].base.address = addr;
                set->results_16[set->results_count].base.prev_i = result_i;
                set->results_16[set->results_count].u16 = *(uint16_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeU32:
                addr = last_set->results_32[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 4) != 4) {
                    continue;
                }
                if(*(uint32_t*)value_buf < value_u32_min ||
                   *(uint32_t*)value_buf > value_u32_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_32[set->results_count].base.address = addr;
                set->results_32[set->results_count].base.prev_i = result_i;
                set->results_32[set->results_count].u32 = *(uint32_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeU64:
                addr = last_set->results_64[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 8) != 8) {
                    continue;
                }
                if(*(uint64_t*)value_buf < value_u64_min ||
                   *(uint64_t*)value_buf > value_u64_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_64[set->results_count].base.address = addr;
                set->results_64[set->results_count].base.prev_i = result_i;
                set->results_64[set->results_count].u64 = *(uint64_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeI8:
                addr = last_set->results_8[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 1) != 1) {
                    continue;
                }
                if(*(int8_t*)value_buf < value_i8_min || *(int8_t*)value_buf > value_i8_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_8[set->results_count].base.address = addr;
                set->results_8[set->results_count].base.prev_i = result_i;
                set->results_8[set->results_count].i8 = *(int8_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeI16:
                addr = last_set->results_16[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 2) != 2) {
                    continue;
                }
                if(*(int16_t*)value_buf < value_i16_min || *(int16_t*)value_buf > value_i16_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_16[set->results_count].base.address = addr;
                set->results_16[set->results_count].base.prev_i = result_i;
                set->results_16[set->results_count].i16 = *(int16_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeI32:
                addr = last_set->results_32[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 4) != 4) {
                    continue;
                }
                if(*(int32_t*)value_buf < value_i32_min || *(int32_t*)value_buf > value_i32_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_32[set->results_count].base.address = addr;
                set->results_32[set->results_count].base.prev_i = result_i;
                set->results_32[set->results_count].i32 = *(int32_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeI64:
                addr = last_set->results_64[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 8) != 8) {
                    continue;
                }
                if(*(int64_t*)value_buf < value_i64_min || *(int64_t*)value_buf > value_i64_max) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_64[set->results_count].base.address = addr;
                set->results_64[set->results_count].base.prev_i = result_i;
                set->results_64[set->results_count].i64 = *(int64_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeF32:
                addr = last_set->results_32[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 4) != 4) {
                    continue;
                }
                if(isnanf(*(flt32_t*)value_buf) || *(flt32_t*)value_buf < (value_f32_min) ||
                   *(flt32_t*)value_buf > (value_f32_max)) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_32[set->results_count].base.address = addr;
                set->results_32[set->results_count].base.prev_i = result_i;
                set->results_32[set->results_count].f32 = *(flt32_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeF64:
                addr = last_set->results_64[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 8) != 8) {
                    continue;
                }
                if(isnanf(*(flt64_t*)value_buf) || *(flt64_t*)value_buf < (value_f64_min) ||
                   *(flt64_t*)value_buf > (value_f64_max)) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_64[set->results_count].base.address = addr;
                set->results_64[set->results_count].base.prev_i = result_i;
                set->results_64[set->results_count].f64 = *(flt64_t*)value_buf;
                set->results_count++;
                break;
            case MemoryTypeF128:
                addr = last_set->results_128[result_i].base.address;
                if(process_handle_read(handle, addr, value_buf, 16) != 16) {
                    continue;
                }
                if(isnanf(*(flt128_t*)value_buf) || *(flt128_t*)value_buf < (value_f128_min) ||
                   *(flt128_t*)value_buf > (value_f128_max)) {
                    continue;
                }
                memory_search_extend_results(set, &capacities[set_i]);
                set->results_128[set->results_count].base.address = addr;
                set->results_128[set->results_count].base.prev_i = result_i;
                set->results_128[set->results_count].f128 = *(flt128_t*)value_buf;
                set->results_count++;
                break;
            default:
                unreachable();
            }
        }

        results_progress += last_set->results_count;
    }
    thread_self_pop_cancel_cleanup(true); // free(value_buf)

    thread_self_pop_cancel_cleanup(true); // memory_search_consolidate_results(memory_search)
    return NULL;
}

static void* memory_search_update_callback(void* context) {
    thread_self_enable_canceling();
    MemorySearch* memory_search = context;

    MemorySearchScratchpad* scratchpad = &memory_search->scratchpad;
    ProcessHandle* handle = memory_search->handle;
    MemorySearchResultBatch* batch = NULL;
    size_t max_type_size = 0;
    if(memory_search->results.batches_count != 0) {
        batch = &memory_search->results.batches[memory_search->results.batches_count - 1];
        for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
            MemorySearchResultSet* set = &batch->sets[set_i];
            MemoryType type = set->type;
            max_type_size = MAX(max_type_size, memory_type_get_size(type));
        }
    }

    while(true) {
        for(size_t item_i = 0; item_i < scratchpad->items_count; item_i++) {
            MemorySearchScratchpadItem* item = &scratchpad->items[item_i];
            size_t size = memory_type_get_size(item->type);
            if(item->active) {
                process_handle_write(handle, item->address, &item->value, size);
            }
            if(process_handle_read(handle, item->address, &item->value, size) != size) {
                memset(&item->value, 0, size);
            }

            thread_self_usleep(update_delay_usec);
        }

        if(batch == NULL || batch->total_results_count >= memory_search_update_max_results) {
            continue;
        }

        for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
            MemorySearchResultSet* set = &batch->sets[set_i];
            size_t size = memory_type_get_size(set->type);
            for(size_t result_i = 0; result_i < set->results_count; result_i++) {
                MemorySearchResultBase* base = memory_search_get_result_base(set, result_i);
                if(process_handle_read(handle, base->address, &base->value, size) != size) {
                    memset(&base->value, 0, size);
                }

                thread_self_usleep(update_delay_usec);
            }
        }
    }

    return NULL;
}

void memory_search_begin(MemorySearch* memory_search) {
    if(!memory_search_process_is_attached(memory_search)) {
        return;
    }
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    if(memory_search->results.batches_count > 0) {
        return;
    }

    memory_search->search_progress = 0.0f;
    memory_search->search_thread = thread_start(memory_search_begin_callback, memory_search);
}

void memory_search_next(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        return;
    }
    if(memory_search->results.batches_count == 0) {
        return;
    }
    if(memory_search->results.current_results_count == 0) {
        return;
    }

    memory_search->search_progress = 0.0f;
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
    thread_cancel(memory_search->search_thread);
}

void memory_search_undo(MemorySearch* memory_search) {
    memory_search_stop_update(memory_search);
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
    memory_search_stop_update(memory_search);
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

MemorySearchResultBase* memory_search_get_result_base(MemorySearchResultSet* set, size_t i) {
    switch(set->type) {
    case MemoryTypeUnsigned:
    case MemoryTypeSigned:
    case MemoryTypeInteger:
    case MemoryTypeFloating:
    case MemoryTypeNumber:
    case MemoryTypeMAX:
        unreachable();

    case MemoryTypeU8: {
        return &set->results_8[i].base;
    }
    case MemoryTypeU16: {
        return &set->results_16[i].base;
    }
    case MemoryTypeU32: {
        return &set->results_32[i].base;
    }
    case MemoryTypeU64: {
        return &set->results_64[i].base;
    }
    case MemoryTypeI8: {
        return &set->results_8[i].base;
    }
    case MemoryTypeI16: {
        return &set->results_16[i].base;
    }
    case MemoryTypeI32: {
        return &set->results_32[i].base;
    }
    case MemoryTypeI64: {
        return &set->results_64[i].base;
    }
    case MemoryTypeF32: {
        return &set->results_32[i].base;
    }
    case MemoryTypeF64: {
        return &set->results_64[i].base;
    }
    case MemoryTypeF128: {
        return &set->results_128[i].base;
    }
    }
}

static MemorySearchResultDisplay
    memory_search_get_display(MemoryAddress address, MemoryType type, void* value) {
    MemorySearchResultDisplay display;

    switch(type) {
    case MemoryTypeUnsigned:
    case MemoryTypeSigned:
    case MemoryTypeInteger:
    case MemoryTypeFloating:
    case MemoryTypeNumber:
    case MemoryTypeMAX:
        unreachable();

    case MemoryTypeU8: {
        snprintf(display.value_str, sizeof(display.value_str), "%hhu", *(uint8_t*)value);
        break;
    }
    case MemoryTypeU16: {
        snprintf(display.value_str, sizeof(display.value_str), "%hu", *(uint16_t*)value);
        break;
    }
    case MemoryTypeU32: {
        snprintf(display.value_str, sizeof(display.value_str), "%u", *(uint32_t*)value);
        break;
    }
    case MemoryTypeU64: {
        snprintf(display.value_str, sizeof(display.value_str), "%lu", *(uint64_t*)value);
        break;
    }
    case MemoryTypeI8: {
        snprintf(display.value_str, sizeof(display.value_str), "%hhi", *(int8_t*)value);
        break;
    }
    case MemoryTypeI16: {
        snprintf(display.value_str, sizeof(display.value_str), "%hi", *(int16_t*)value);
        break;
    }
    case MemoryTypeI32: {
        snprintf(display.value_str, sizeof(display.value_str), "%i", *(int32_t*)value);
        break;
    }
    case MemoryTypeI64: {
        snprintf(display.value_str, sizeof(display.value_str), "%li", *(int64_t*)value);
        break;
    }
    case MemoryTypeF32: {
        snprintf(display.value_str, sizeof(display.value_str), "%.*g", FLT_DIG, *(flt32_t*)value);
        break;
    }
    case MemoryTypeF64: {
        snprintf(display.value_str, sizeof(display.value_str), "%.*lg", DBL_DIG, *(flt64_t*)value);
        break;
    }
    case MemoryTypeF128: {
        snprintf(
            display.value_str,
            sizeof(display.value_str),
            "%.*Lg",
            LDBL_DIG,
            *(flt128_t*)value);
        break;
    }
    }

    snprintf(display.address_str, sizeof(display.address_str), "0x%" PRIXPTR, (uintptr_t)address);

    return display;
}

MemorySearchResultDisplay memory_search_get_result_display(MemorySearchResultSet* set, size_t i) {
    MemorySearchResultBase* base = memory_search_get_result_base(set, i);
    return memory_search_get_display(base->address, set->type, &base->value);
}

void memory_search_scratchpad_add(MemorySearch* memory_search, MemoryAddress addr, MemoryType type) {
    memory_search_stop_update(memory_search);
    if(!memory_search_process_is_attached(memory_search)) {
        return;
    }
    if(memory_search_is_searching(memory_search)) {
        return;
    }

    ProcessRegion* region = NULL;
    ProcessRegions* regions = memory_search->results.regions;
    for(size_t region_i = 0; region_i < regions->regions_count; region_i++) {
        ProcessRegion* reg = &regions->regions[region_i];
        if(addr >= reg->start && addr < reg->end) {
            region = reg;
            break;
        }
    }

    MemorySearchScratchpad* scratchpad = &memory_search->scratchpad;
    if(scratchpad->items_count > 0) {
        for(size_t item_i = 0; item_i < scratchpad->items_count; item_i++) {
            MemorySearchScratchpadItem* item = &scratchpad->items[item_i];
            if(item->address == addr && item->type == type) {
                return;
            }
        }
        scratchpad->items = realloc(
            scratchpad->items,
            sizeof(MemorySearchScratchpadItem) * (scratchpad->items_count + 1));
    } else {
        scratchpad->items = malloc(sizeof(MemorySearchScratchpadItem) * 1);
    }

    MemorySearchScratchpadItem* item = &scratchpad->items[scratchpad->items_count];
    item->address = addr;
    item->type = type;
    item->active = false;
    item->description_len = 1;
    item->description = malloc(item->description_len);
    item->description[0] = '\0';
    if(region != NULL) {
        item->region.type = region->type;
        item->region.flags = region->flags;
        if(region->file_path != NULL) {
            item->region.file_path = strdup(region->file_path);
        } else {
            item->region.file_path = NULL;
        }
        item->region.file_offset = region->file_offset + (addr - region->start);
    } else {
        item->region.type = 0;
        item->region.flags = 0;
        item->region.file_path = NULL;
        item->region.file_offset = 0;
    }
    size_t size = memory_type_get_size(type);
    if(process_handle_read(memory_search->handle, addr, &item->value, size) != size) {
        memset(&item->value, 0, size);
    }
    scratchpad->items_count++;
}

MemorySearchScratchpad* memory_search_get_scratchpad(MemorySearch* memory_search) {
    return &memory_search->scratchpad;
}

MemorySearchResultDisplay memory_search_get_scratchpad_display(MemorySearchScratchpadItem* item) {
    return memory_search_get_display(item->address, item->type, &item->value);
}

void memory_search_scratchpad_set(
    MemorySearch* memory_search,
    MemorySearchScratchpadItem* item,
    flt128_t value) {
    memory_search_stop_update(memory_search);
    if(!memory_search_process_is_attached(memory_search)) {
        return;
    }
    if(memory_search_is_searching(memory_search)) {
        return;
    }

    switch(item->type) {
    case MemoryTypeUnsigned:
    case MemoryTypeSigned:
    case MemoryTypeInteger:
    case MemoryTypeFloating:
    case MemoryTypeNumber:
    case MemoryTypeMAX:
        unreachable();

    case MemoryTypeU8: {
        uint8_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeU16: {
        uint16_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeU32: {
        uint32_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeU64: {
        uint64_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeI8: {
        int8_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeI16: {
        int16_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeI32: {
        int32_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeI64: {
        int64_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeF32: {
        flt32_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeF64: {
        flt64_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    case MemoryTypeF128: {
        flt128_t val = value;
        process_handle_write(memory_search->handle, item->address, &val, sizeof(val));
        break;
    }
    }

    size_t size = memory_type_get_size(item->type);
    if(process_handle_read(memory_search->handle, item->address, &item->value, size) != size) {
        memset(&item->value, 0, size);
    }
}

void memory_search_scratchpad_del(MemorySearch* memory_search, MemorySearchScratchpadItem* item) {
    memory_search_stop_update(memory_search);
    if(!memory_search_process_is_attached(memory_search)) {
        return;
    }
    if(memory_search_is_searching(memory_search)) {
        return;
    }

    MemorySearchScratchpad* scratchpad = &memory_search->scratchpad;
    if(scratchpad->items_count == 0) {
        return;
    }
    size_t item_i;
    for(item_i = 0; item_i < scratchpad->items_count; item_i++) {
        MemorySearchScratchpadItem* del_item = &scratchpad->items[item_i];
        if(del_item == item) {
            break;
        }
    }
    if(item_i == scratchpad->items_count) {
        return;
    }

    free(item->description);
    if(item->region.file_path != NULL) {
        free(item->region.file_path);
    }

    scratchpad->items_count--;
    if(scratchpad->items_count == 0) {
        free(scratchpad->items);
        scratchpad->items = NULL;
    } else {
        memmove(
            &scratchpad->items[item_i],
            &scratchpad->items[item_i + 1],
            sizeof(MemorySearchScratchpadItem) * (scratchpad->items_count - item_i));
        scratchpad->items = realloc(
            scratchpad->items,
            sizeof(MemorySearchScratchpadItem) * scratchpad->items_count);
    }
}

void memory_search_scratchpad_wipe(MemorySearch* memory_search) {
    memory_search_stop_update(memory_search);
    if(memory_search_is_searching(memory_search)) {
        return;
    }

    size_t items_count = memory_search->scratchpad.items_count;
    MemorySearchScratchpadItem* items = memory_search->scratchpad.items;
    memory_search->scratchpad.items_count = 0;
    memory_search->scratchpad.items = NULL;
    if(items_count >= 1) {
        for(size_t item_i = 0; item_i < items_count; item_i++) {
            MemorySearchScratchpadItem* item = &items[item_i];
            free(item->description);
            if(item->region.file_path != NULL) {
                free(item->region.file_path);
            }
        }
        free(items);
    }
}

void memory_search_tick(MemorySearch* memory_search) {
    if(memory_search_is_searching(memory_search)) {
        if(thread_try_join(memory_search->search_thread, NULL)) {
            memory_search->search_thread = NULL;
        }
    } else if(memory_search_process_is_attached(memory_search)) {
        if(!process_handle_is_valid(memory_search->handle)) {
            memory_search_process_detach(memory_search);
        } else if(
            memory_search->update_thread == NULL &&
            ((memory_search->results.current_results_count > 0 &&
              memory_search->results.current_results_count < memory_search_update_max_results) ||
             memory_search->scratchpad.items_count > 0)) {
            memory_search->update_thread =
                thread_start(memory_search_update_callback, memory_search);
        }
    }
}

void memory_search_free(MemorySearch* memory_search) {
    memory_search_stop_update(memory_search);
    if(memory_search_is_searching(memory_search)) {
        memory_search_stop(memory_search);
        thread_join(memory_search->search_thread, NULL);
    }
    if(memory_search_process_is_attached(memory_search)) {
        memory_search_process_detach(memory_search);
    }
    free(memory_search);
}
