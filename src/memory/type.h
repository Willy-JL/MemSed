#pragma once

#include <std.h>

typedef enum {
    MemoryTypeU8,
    MemoryTypeU16,
    MemoryTypeU32,
    MemoryTypeU64,
    MemoryTypeUnsigned,
    MemoryTypeI8,
    MemoryTypeI16,
    MemoryTypeI32,
    MemoryTypeI64,
    MemoryTypeSigned,
    MemoryTypeInteger,
    MemoryTypeF32,
    MemoryTypeF64,
    MemoryTypeF128,
    MemoryTypeFloating,
    MemoryTypeNumber,
    MemoryTypeMAX,
} MemoryType;

extern const char* memory_type_names[MemoryTypeMAX];

const char* memory_type_get_name(MemoryType type);
size_t memory_type_get_size(MemoryType type);
