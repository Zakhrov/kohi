#pragma once

#include "../defines.h"

typedef struct LinearAllocator{
    u64 totalSize;
    u64 allocated;
    void* memory;
    b8 ownsMemory;
}LinearAllocator;

KAPI void linear_allocator_create(u64 totalSize, void* memory, LinearAllocator* allocator);

KAPI void linear_allocator_destroy(LinearAllocator* allocator);

KAPI void* linear_allocator_allocate(LinearAllocator* allocator, u64 size);
KAPI void linear_allocator_free_all(LinearAllocator* allocator);