#include "type.h"

const char* memory_type_names[MemoryTypeMAX] = {
    [MemoryTypeU8] = "8-bit unsigned integer (u8)",
    [MemoryTypeU16] = "16-bit unsigned integer (u16)",
    [MemoryTypeU32] = "32-bit unsigned integer (u32)",
    [MemoryTypeU64] = "64-bit unsigned integer (u64)",
    [MemoryTypeUnsigned] = "Unsigned integer (any size)",
    [MemoryTypeI8] = "8-bit signed integer (i8)",
    [MemoryTypeI16] = "16-bit signed integer (i16)",
    [MemoryTypeI32] = "32-bit signed integer (i32)",
    [MemoryTypeI64] = "64-bit signed integer (i64)",
    [MemoryTypeSigned] = "Signed integer (any size)",
    [MemoryTypeInteger] = "Integer (any size and signedness)",
    [MemoryTypeF32] = "32-bit float (f32)",
    [MemoryTypeF64] = "64-bit double (f64)",
    [MemoryTypeF128] = "128-bit long double (f128)",
    [MemoryTypeFloating] = "Floating point (any size)",
    [MemoryTypeNumber] = "Number (all types)",
};

const char* memory_type_get_name(MemoryType type) {
    assert(type < MemoryTypeMAX);
    return memory_type_names[type];
}

static const size_t memory_type_sizes[MemoryTypeMAX] = {
    [MemoryTypeU8] = 1,
    [MemoryTypeU16] = 2,
    [MemoryTypeU32] = 4,
    [MemoryTypeU64] = 8,
    [MemoryTypeUnsigned] = 8,
    [MemoryTypeI8] = 1,
    [MemoryTypeI16] = 2,
    [MemoryTypeI32] = 4,
    [MemoryTypeI64] = 8,
    [MemoryTypeSigned] = 8,
    [MemoryTypeF32] = 4,
    [MemoryTypeF64] = 8,
    [MemoryTypeF128] = 16,
    [MemoryTypeFloating] = 16,
    [MemoryTypeNumber] = 16,
};

size_t memory_type_get_size(MemoryType type) {
    assert(type < MemoryTypeMAX);
    return memory_type_sizes[type];
}
