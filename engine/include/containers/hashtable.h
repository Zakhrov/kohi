#pragma once

#include "../defines.h"


/**
 * @brief Represents a simple HashTable. Members of this structure
 * should not be modified outside the functions associated with it.
 * 
 * For non-pointer types, table retains a copy of the value.For 
 * pointer types, make sure to use the _ptr setter and getter. Table
 * does not take ownership of pointers or associated memory allocations,
 * and should be managed externally.
 */
typedef struct HashTable{
    u64 elementSize;
    u64 elementCount;
    b8 isPointerType;
    void* memory;

}HashTable;
/**
 * @brief Creates a HashTable and stores it in out_hashtable.
 * 
 * @param element_size The size of each element in bytes.
 * @param element_count The maximum number of elements. Cannot be resized.
 * @param memory A block of memory to be used. Should be equal in size to element_size * element_count;
 * @param is_pointer_type Indicates if this HashTable will hold pointer types.
 * @param out_hashtable A pointer to a HashTable in which to hold relevant data.
 */
KAPI void hashtable_create(u64 element_size, u32 element_count, void* memory, b8 is_pointer_type, HashTable* out_hashtable);

/**
 * @brief Destroys the provided HashTable. Does not release memory for pointer types.
 * 
 * @param table A pointer to the table to be destroyed.
 */
KAPI void hashtable_destroy(HashTable* table);

/**
 * @brief Stores a copy of the data in value in the provided HashTable. 
 * Only use for tables which were *NOT* created with is_pointer_type = true.
 * 
 * @param table A pointer to the table to get from. Required.
 * @param name The name of the entry to set. Required.
 * @param value The value to be set. Required.
 * @return True, or false if a null pointer is passed.
 */
KAPI b8 hashtable_set(HashTable* table, const char* name, void* value);

/**
 * @brief Stores a pointer as provided in value in the HashTable.
 * Only use for tables which were created with is_pointer_type = true.
 * 
 * @param table A pointer to the table to get from. Required.
 * @param name The name of the entry to set. Required.
 * @param value A pointer value to be set. Can pass 0 to 'unset' an entry.
 * @return True; or false if a null pointer is passed or if the entry is 0.
 */
KAPI b8 hashtable_set_ptr(HashTable* table, const char* name, void** value);

/**
 * @brief Obtains a copy of data present in the HashTable.
 * Only use for tables which were *NOT* created with is_pointer_type = true.
 * 
 * @param table A pointer to the table to retrieved from. Required.
 * @param name The name of the entry to retrieved. Required.
 * @param value A pointer to store the retrieved value. Required.
 * @return True; or false if a null pointer is passed.
 */
KAPI b8 hashtable_get(HashTable* table, const char* name, void* out_value);

/**
 * @brief Obtains a pointer to data present in the HashTable.
 * Only use for tables which were created with is_pointer_type = true.
 * 
 * @param table A pointer to the table to retrieved from. Required.
 * @param name The name of the entry to retrieved. Required.
 * @param value A pointer to store the retrieved value. Required.
 * @return True if retrieved successfully; false if a null pointer is passed or is the retrieved value is 0.
 */
KAPI b8 hashtable_get_ptr(HashTable* table, const char* name, void** out_value);

/**
 * @brief Fills all entries in the HashTable with the given value.
 * Useful when non-existent names should return some default value.
 * Should not be used with pointer table types.
 * 
 * @param table A pointer to the table filled. Required.
 * @param value The value to be filled with. Required.
 * @return True if successful; otherwise false.
 */
KAPI b8 hashtable_fill(HashTable* table, void* value);
