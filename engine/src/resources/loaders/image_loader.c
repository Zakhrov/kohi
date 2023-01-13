#include "resources/loaders/image_loader.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

b8 image_loader_load(ResourceLoader* self,const char* name,Resource* resource){
     if (!self || !name || !resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: try different extensions
    string_format(full_file_path, format_str, resource_system_base_path(), self->typePath, name, ".png");

    i32 width;
    i32 height;
    i32 channel_count;

    // For now, assume 8 bits per channel, 4 channels.
    // TODO: extend this to make it configurable.
    u8* data = stbi_load(
        full_file_path,
        &width,
        &height,
        &channel_count,
        required_channel_count);

    // Check for a failure reason. If there is one, abort, clear memory if allocated, return false.
    const char* fail_reason = stbi_failure_reason();
    if (fail_reason) {
        KERROR("Image resource loader failed to load file '%s': %s", full_file_path, fail_reason);
        // Clear the error so the next load doesn't fail.
        stbi__err(0, 0);

        if (data) {
            stbi_image_free(data);
        }
        return false;
    }

    if (!data) {
        KERROR("Image resource loader failed to load file '%s'.", full_file_path);
        return false;
    }

    // TODO: Should be using an allocator here.
    resource->fullPath = string_duplicate(full_file_path);

    // TODO: Should be using an allocator here.
    ImageResourceData* resourceData = kallocate(sizeof(ImageResourceData), MEMORY_TAG_TEXTURE);
    resourceData->pixels = data;
    resourceData->width = width;
    resourceData->height = height;
    resourceData->channelCount = required_channel_count;

    resource->data = resourceData;
    resource->dataSize = sizeof(ImageResourceData);
    resource->name = name;

    return true;
}

void image_loader_unload(ResourceLoader* self, Resource* resource) {
    if (!self || !resource) {
        KWARN("image_loader_unload called with nullptr for self or resource.");
        return;
    }

    u32 path_length = string_length(resource->fullPath);
    if (path_length) {
        kfree(resource->fullPath, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        kfree(resource->data, resource->dataSize, MEMORY_TAG_TEXTURE);
        resource->data = 0;
        resource->dataSize = 0;
        resource->loaderId = INVALID_ID;
    }
}


ResourceLoader image_resource_loader_create(){
    ResourceLoader loader;
    loader.type = RESOURCE_TYPE_IMAGE;
    loader.customType = 0;
    loader.load = image_loader_load;
    loader.unload = image_loader_unload;
    loader.typePath = "textures";
    return loader;
}