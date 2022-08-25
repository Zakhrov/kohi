#include "memory/linear_allocator.h"
#include "memory/kmemory.h"
#include "core/logger.h"


void linear_allocator_create(u64 totalSize, void* memory, LinearAllocator* allocator){
     if (allocator) {
        allocator->totalSize = totalSize;
        allocator->allocated = 0;
        allocator->ownsMemory = memory == 0;
        if (memory) {
            allocator->memory = memory;
        } else {
            allocator->memory = kallocate(totalSize, MEMORY_TAG_LINEAR_ALLOCATOR);
        }
    }

}

void linear_allocator_destroy(LinearAllocator* allocator){
    if (allocator) {
        allocator->allocated = 0;
        if (allocator->ownsMemory && allocator->memory) {
            kfree(allocator->memory, allocator->totalSize, MEMORY_TAG_LINEAR_ALLOCATOR);
        } 
        allocator->memory = 0;
        allocator->totalSize = 0;
        allocator->ownsMemory = false;
    }

}

KAPI void* linear_allocator_allocate(LinearAllocator* allocator, u64 size){

    if (allocator && allocator->memory) {
        if (allocator->allocated + size > allocator->totalSize) {
            u64 remaining = allocator->totalSize - allocator->allocated;
            KERROR("linear_allocator_allocate - Tried to allocate %lluB, only %lluB remaining.", size, remaining);
            return 0;
        }

        void* block = ((u8*)allocator->memory) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    KERROR("linear_allocator_allocate - provided allocator not initialized.");
    return 0;

}
KAPI void linear_allocator_free_all(LinearAllocator* allocator){
    if (allocator && allocator->memory) {
        allocator->allocated = 0;
        kzero_memory(allocator->memory, allocator->totalSize);
    }

}