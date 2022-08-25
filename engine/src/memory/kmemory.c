#include "memory/kmemory.h"

#include "core/logger.h"
#include "platform/platform.h"

#include <string.h>
#include <stdio.h>

struct MemoryStats {
    u64 totalAllocated;
    u64 taggedAllocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memoryTagStrings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN         ",
    "ARRAY           ",
    "LINEAR_ALLOCATOR",
    "DARRAY          ",
    "DICT            ",
    "RING_QUEUE      ",
    "BST             ",
    "STRING          ",
    "APPLICATION     ",
    "JOB             ",
    "TEXTURE         ",
    "MAT_INST        ",
    "RENDERER        ",
    "GAME            ",
    "TRANSFORM       ",
    "ENTITY          ",
    "ENTITY_NODE     ",
    "SCENE           "};


typedef struct MemorySystemState { 
    struct MemoryStats stats;
    u64 allocationCount;
} MemorySystemState;
static MemorySystemState* statePtr;

void memory_system_initialize(u64* memoryRequirements, void* state) {
    *memoryRequirements = sizeof(MemorySystemState);
    if(state == 0){
        return;

    }
    platform_zero_memory(state,sizeof(MemorySystemState));
    statePtr = state;
    statePtr->allocationCount = 0;
    platform_zero_memory(&statePtr->stats, sizeof(statePtr->stats));
    KDEBUG("MEMORY SUBSYSTEM INITIALIZED");
}

void memory_system_shutdown(void* state) {
    // platform_free(statePtr,false);
    statePtr = 0;
}

void* kallocate(u64 size, MemoryTag tag){
     if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    if(statePtr){
        statePtr->allocationCount++;
        statePtr->stats.totalAllocated += size;
        statePtr->stats.taggedAllocations[tag] += size;
    }

    // TODO: Memory alignment
    void* block = platform_allocate(size, false);
    platform_zero_memory(block, size);
    return block;

}

u64 get_memory_alloc_count(){
    if(statePtr){
        return statePtr->allocationCount;
    }
    return 0;
}

void kfree(void* block, u64 size, MemoryTag tag){
     if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }
    if(statePtr){
        statePtr->stats.totalAllocated -= size;
        statePtr->stats.taggedAllocations[tag] -= size;
    }
    

    // TODO: Memory alignment
    platform_free(block, false);

}

void* kzero_memory(void* block, u64 size){
    return platform_zero_memory(block, size);

}

void* kcopy_memory(void* dest, const void* source, u64 size){
    return platform_copy_memory(dest, source, size);

}

void* kset_memory(void* dest, i32 value, u64 size){
    return platform_set_memory(dest, value, size);

}

char* get_memory_usage_str(){
      const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (statePtr->stats.taggedAllocations[i] >= gib) {
            unit[0] = 'G';
            amount = statePtr->stats.taggedAllocations[i] / (float)gib;
        } else if (statePtr->stats.taggedAllocations[i] >= mib) {
            unit[0] = 'M';
            amount = statePtr->stats.taggedAllocations[i] / (float)mib;
        } else if (statePtr->stats.taggedAllocations[i] >= kib) {
            unit[0] = 'K';
            amount = statePtr->stats.taggedAllocations[i] / (float)kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)statePtr->stats.taggedAllocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memoryTagStrings[i], amount, unit);
        offset += length;
    }
    char* out_string = strdup(buffer);
    return out_string;

}
