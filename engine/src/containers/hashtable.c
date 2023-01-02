#include "containers/hashtable.h"

#include "memory/kmemory.h"
#include "core/logger.h"

u64 hash_name(const char* name, u32 elementCount) {
    // A multipler to use when generating a hash. Prime to hopefully avoid collisions.
    static const u64 multiplier = 97;

    unsigned const char* us;
    u64 hash = 0;

    for (us = (unsigned const char*)name; *us; us++) {
        hash = hash * multiplier + *us;
    }

    // Mod it against the size of the table.
    hash %= elementCount;

    return hash;
}

void hashtable_create(u64 elementSize, u32 elementCount, void* memory, b8 isPointerType, HashTable* out_hashtable) {
    if (!memory || !out_hashtable) {
        KERROR("hashtable_create failed! Pointer to memory and out_hashtable are required.");
        return;
    }
    if (!elementCount || !elementSize) {
        KERROR("elementSize and elementCount must be a positive non-zero value.");
        return;
    }

    // TODO: Might want to require an allocator and allocate this memory instead.
    out_hashtable->memory = memory;
    out_hashtable->elementCount = elementCount;
    out_hashtable->elementSize = elementSize;
    out_hashtable->isPointerType = isPointerType;
    kzero_memory(out_hashtable->memory, elementSize * elementCount);
}

void hashtable_destroy(HashTable* table) {
    if (table) {
        // TODO: If using allocator above, free memory here.
        kzero_memory(table, sizeof(HashTable));
    }
}

b8 hashtable_set(HashTable* table, const char* name, void* value) {
    if (!table || !name || !value) {
        KERROR("hashtable_set requires table, name and value to exist.");
        return false;
    }
    if (table->isPointerType) {
        KERROR("hashtable_set should not be used with tables that have pointer types. Use hashtable_set_ptr instead.");
        return false;
    }

    u64 hash = hash_name(name, table->elementCount);
    kcopy_memory(table->memory + (table->elementSize * hash), value, table->elementSize);
    return true;
}

b8 hashtable_set_ptr(HashTable* table, const char* name, void** value) {
    if (!table || !name) {
        KWARN("hashtable_set_ptr requires table and name  to exist.");
        return false;
    }
    if (!table->isPointerType) {
        KERROR("hashtable_set_ptr should not be used with tables that do not have pointer types. Use hashtable_set instead.");
        return false;
    }

    u64 hash = hash_name(name, table->elementCount);
    ((void**)table->memory)[hash] = value ? *value : 0;
    return true;
}

b8 hashtable_get(HashTable* table, const char* name, void* out_value) {
    if (!table || !name || !out_value) {
        KWARN("hashtable_get requires table, name and out_value to exist.");
        return false;
    }
    if (table->isPointerType) {
        KERROR("hashtable_get should not be used with tables that have pointer types. Use hashtable_set_ptr instead.");
        return false;
    }
    u64 hash = hash_name(name, table->elementCount);
    kcopy_memory(out_value, table->memory + (table->elementSize * hash), table->elementSize);
    return true;
}

b8 hashtable_get_ptr(HashTable* table, const char* name, void** out_value) {
    if (!table || !name || !out_value) {
        KWARN("hashtable_get_ptr requires table, name and out_value to exist.");
        return false;
    }
    if (!table->isPointerType) {
        KERROR("hashtable_get_ptr should not be used with tables that do not have pointer types. Use hashtable_get instead.");
        return false;
    }

    u64 hash = hash_name(name, table->elementCount);
    *out_value = ((void**)table->memory)[hash];
    return *out_value != 0;
}

b8 hashtable_fill(HashTable* table, void* value) {
    if (!table || !value) {
        KWARN("hashtable_fill requires table and value to exist.");
        return false;
    }
    if (table->isPointerType) {
        KERROR("hashtable_fill should not be used with tables that have pointer types.");
        return false;
    }

    for (u32 i = 0; i < table->elementCount; ++i) {
        kcopy_memory(table->memory + (table->elementSize * i), value, table->elementSize);
    }

    return true;
}