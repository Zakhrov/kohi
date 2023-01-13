#include "resources/loaders/binary_loader.h"
#include "core/logger.h"

#include "memory/kmemory.h"

#include "core/kstring.h"

#include "resources/resource_types.h"

#include "systems/resource_system.h"

#include "math/kmath.h"




#include "platform/filesystem.h"




b8 binary_loader_load( ResourceLoader* self, const char* name, Resource* resource) {
    KDEBUG("COMING IN BINARY LOADER");

    if (!self || !name || !resource) {

        return false;

    }




    char* format_str = "%s%s/%s%s";

    char full_file_path[512];

    string_format(full_file_path, format_str, resource_system_base_path(), self->typePath, name, "");
    KDEBUG("BINARY FILE PATH %s",full_file_path);




    // TODO: Should be using an allocator here.

    resource->fullPath = string_duplicate(full_file_path);




    FileHandle f;

    if (!filesystem_open(full_file_path, FILE_MODE_READ, true, &f)) {

        KERROR("binary_loader_load - unable to open file for binary reading: '%s'.", full_file_path);

        return false;

    }




    u64 file_size = 0;

    if (!filesystem_size(&f, &file_size)) {

        KERROR("Unable to binary read file: %s.", full_file_path);

        filesystem_close(&f);

        return false;

    }




    // TODO: Should be using an allocator here.

    u8* resource_data = kallocate(sizeof(u8) * file_size, MEMORY_TAG_ARRAY);

    u64 read_size = 0;

    if (!filesystem_read_all_bytes(&f, resource_data, &read_size)) {

        KERROR("Unable to binary read file: %s.", full_file_path);

        filesystem_close(&f);

        return false;

    }




    filesystem_close(&f);




    resource->data = resource_data;

    resource->dataSize = read_size;

    resource->name = name;




    return true;

}




void binary_loader_unload(ResourceLoader* self, Resource* resource) {

    if (!self || !resource) {

        KWARN("binary_loader_unload called with nullptr for self or resource.");

        return;

    }




    u32 path_length = string_length(resource->fullPath);

    if (path_length) {

        kfree(resource->fullPath, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);

    }




    if (resource->data) {

        kfree(resource->data, resource->dataSize, MEMORY_TAG_ARRAY);

        resource->data = 0;

        resource->dataSize = 0;

        resource->loaderId = INVALID_ID;

    }

}




ResourceLoader binary_resource_loader_create() {

    ResourceLoader loader;

    loader.type = RESOURCE_TYPE_BINARY;

    loader.customType = 0;

    loader.load = binary_loader_load;

    loader.unload = binary_loader_unload;

    loader.typePath = "";

    // KDEBUG("Creating Binary Loader");




    return loader;

}