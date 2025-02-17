#pragma once

#include "../memory/address.h"
#include "pid.h"

typedef enum {
    ProcessRegionTypeFile = 1 << 0,
    ProcessRegionTypeHeap = 1 << 1,
    ProcessRegionTypeStack = 1 << 2,
    ProcessRegionTypeAnonymous = 1 << 3,
} ProcessRegionType;

typedef enum {
    ProcessRegionFlagRead = 1 << 0,
    ProcessRegionFlagWrite = 1 << 1,
    ProcessRegionFlagExecute = 1 << 2,
    ProcessRegionFlagShared = 1 << 3,
} ProcessRegionFlag;

typedef struct {
    MemoryAddress start;
    MemoryAddress end;
    ProcessRegionType type;
    ProcessRegionFlag flags;
    char* file_path;
    size_t file_offset;
} ProcessRegion;

typedef struct {
    size_t total_size;
    size_t regions_count;
    ProcessRegion regions[];
} ProcessRegions;

ProcessRegions* process_regions_init(
    ProcessPid pid,
    ProcessRegionType types_mask,
    ProcessRegionFlag flags_mask);
void process_regions_free(ProcessRegions* regions);
