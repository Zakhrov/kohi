#include "systems/texture_system.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

// TODO: Rsource Loader
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"


typedef struct TextureSystemState{
    TextureSystemConfig config;
    Texture defaultTexture;
    // Array of registered textures
    Texture* registeredTextures;
    // Hash table for easy texture lookup
    HashTable registeredTextureTable;

}TextureSystemState;

typedef struct TextureReference{
    u64 referenceCount;
    u32 handle;
    b8 autoRelease;
}TextureReference;

static TextureSystemState* statePtr = 0;



void create_texture(Texture* texture){
    kzero_memory(texture,sizeof(Texture));
    texture->generation = INVALID_ID;
}
b8 create_default_textures(TextureSystemState* state);
void destroy_default_textures(TextureSystemState* state);
b8 load_texture(const char* textureName,Texture* texture);
void destroy_texture(Texture* texture);
b8 texture_system_initialize(u64* memoryRequirement, void* state, TextureSystemConfig config){
    if(config.maxTextureCount == 0){
        KFATAL("Texture System Initialize config.maxxTextureCount must be > 0");
        return false;
    }
    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(TextureSystemState);
    u64 array_requirement = sizeof(Texture) * config.maxTextureCount;
    u64 hashtable_requirement = sizeof(TextureReference) * config.maxTextureCount;
    *memoryRequirement = struct_requirement + array_requirement + hashtable_requirement;

        if (!state) {
        return true;
    }

    statePtr = state;
    statePtr->config = config;

    // The array block is after the state. Already allocated, so just set the pointer.
    void* array_block = state + struct_requirement;
    statePtr->registeredTextures = array_block;

    // Hashtable block is after array.
    void* hashtable_block = array_block + array_requirement;

    // Create a hashtable for texture lookups.
    hashtable_create(sizeof(TextureReference), config.maxTextureCount, hashtable_block, false, &statePtr->registeredTextureTable);

    // Fill the hashtable with invalid references to use as a default.
    TextureReference invalid_ref;
    invalid_ref.autoRelease = false;
    invalid_ref.handle = INVALID_ID;  // Primary reason for needing default values.
    invalid_ref.referenceCount = 0;
    hashtable_fill(&statePtr->registeredTextureTable, &invalid_ref);

    // Invalidate all textures in the array.
    u32 count = statePtr->config.maxTextureCount;
    for (u32 i = 0; i < count; ++i) {
        statePtr->registeredTextures[i].id = INVALID_ID;
        statePtr->registeredTextures[i].generation = INVALID_ID;
    }

    // Create default textures for use in the system.
    create_default_textures(statePtr);
    KINFO("Texture System Initialized");

    return true;


}





void texture_system_shutdown(void* state){
    if (statePtr) {
        // Destroy all loaded textures.
        for (u32 i = 0; i < statePtr->config.maxTextureCount; ++i) {
            Texture* t = &statePtr->registeredTextures[i];
            if (t->generation != INVALID_ID) {
                renderer_destroy_texture(t);
            }
        }

        destroy_default_textures(statePtr);

        statePtr = 0;
    }

}

Texture* texture_system_acquire(const char* name, b8 autoRelease){
    // Return default texture, but warn about it since this should be returned via get_default_texture();
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {
        KWARN("texture_system_acquire called for default texture. Use texture_system_get_default_texture for texture 'default'.");
        return &statePtr->defaultTexture;
    }

    TextureReference ref;
    if (statePtr && hashtable_get(&statePtr->registeredTextureTable, name, &ref)) {
        // This can only be changed the first time a texture is loaded.
        if (ref.referenceCount == 0) {
            ref.autoRelease = autoRelease;
        }
        ref.referenceCount++;
        if (ref.handle == INVALID_ID) {
            // This means no texture exists here. Find a free index first.
            u32 count = statePtr->config.maxTextureCount;
            Texture* t = 0;
            for (u32 i = 0; i < count; ++i) {
                if (statePtr->registeredTextures[i].id == INVALID_ID) {
                    // A free slot has been found. Use its index as the handle.
                    ref.handle = i;
                    t = &statePtr->registeredTextures[i];
                    break;
                }
            }

            // Make sure an empty slot was actually found.
            if (!t || ref.handle == INVALID_ID) {
                KFATAL("texture_system_acquire - Texture system cannot hold anymore textures. Adjust configuration to allow more.");
                return 0;
            }

            // Create new texture.
            if (!load_texture(name, t)) {
                KERROR("Failed to load texture '%s'.", name);
                return 0;
            }

            // Also use the handle as the texture id.
            t->id = ref.handle;
            KTRACE("Texture '%s' does not yet exist. Created, and ref_count is now %i.", name, ref.referenceCount);
        } else {
            KTRACE("Texture '%s' already exists, ref_count increased to %i.", name, ref.referenceCount);
        }

        // Update the entry.
        hashtable_set(&statePtr->registeredTextureTable, name, &ref);
        return &statePtr->registeredTextures[ref.handle];
    }

    // NOTE: This would only happen in the event something went wrong with the state.
    KERROR("texture_system_acquire failed to acquire texture '%s'. Null pointer will be returned.", name);
    return 0;
}
void texture_system_release(const char* name){
        // Ignore release requests for the default texture.
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {
        return;
    }
    TextureReference ref;
    if (statePtr && hashtable_get(&statePtr->registeredTextureTable, name, &ref)) {
        if (ref.referenceCount == 0) {
            KWARN("Tried to release non-existent texture: '%s'", name);
            return;
        }
        // Take a copy of the name since it will be wiped out by destroy,
        // (as passed in name is generally a pointer to the actual texture's name).
        char name_copy[TEXTURE_NAME_MAX_LENGTH];
        string_ncopy(name_copy, name, TEXTURE_NAME_MAX_LENGTH);
        ref.referenceCount--;
        if (ref.referenceCount == 0 && ref.autoRelease) {
            Texture* t = &statePtr->registeredTextures[ref.handle];

            destroy_texture(t);
            // Reset the reference.
            ref.handle = INVALID_ID;
            ref.autoRelease = false;
            KTRACE("Released texture '%s'., Texture unloaded because reference count=0 and autoRelease=true.", name_copy);
        } else {
            KTRACE("Released texture '%s', now has a reference count of '%i' (autoRelease=%s).", name_copy, ref.referenceCount, ref.autoRelease ? "true" : "false");
        }

        // Update the entry.
        hashtable_set(&statePtr->registeredTextureTable, name_copy, &ref);
    } else {
        KERROR("texture_system_release failed to release texture '%s'.", name);
    }

}
Texture* texture_system_get_default_texture(){
    if (statePtr) {
        return &statePtr->defaultTexture;
    }

    KERROR("texture_system_get_default_texture called before texture system initialization! Null pointer returned.");
    return 0;

}





b8 create_default_textures(TextureSystemState* state){
    // NOTE: Create default texture, a 256x256 blue/white checkerboard pattern.
    // This is done in code to eliminate asset dependencies.
    KTRACE("Creating default texture...");
    const u32 tex_dimension = 256;
    const u32 channels = 4;
    const u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[pixel_count * channels];
    //u8* pixels = kallocate(sizeof(u8) * pixel_count * bpp, MEMORY_TAG_TEXTURE);
    kset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);

    // Each pixel.
    for (u64 row = 0; row < tex_dimension; ++row) {
        for (u64 col = 0; col < tex_dimension; ++col) {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }
    string_ncopy(state->defaultTexture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->defaultTexture.width = tex_dimension;
    state->defaultTexture.height = tex_dimension;
    state->defaultTexture.channelCount = 4;
    state->defaultTexture.generation = INVALID_ID;
    state->defaultTexture.hasTransparency = false;
    renderer_create_texture(pixels, &statePtr->defaultTexture);
    // Manually set default texture generation to INVALID_ID since this is the default texture
    statePtr->defaultTexture.generation = INVALID_ID;
    KDEBUG("Default Texture generation in Texure System %d",statePtr->defaultTexture.generation);
    
    
}


void destroy_default_textures(TextureSystemState* state){
    if(state){
        destroy_texture(&state->defaultTexture);
    }
}



b8 load_texture(const char* textureName,Texture* texture){
    // TODO: Should be able to be located anywhere
    char* formatStr = "../assets/textures/%s.%s";
    const i32 requiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char fullFilePath[512];
    // TODO: try different extensions
    string_format(fullFilePath,formatStr,textureName,"png");

    // Use a temporary local texture to load everything
    Texture tempTexture;
    u8* data = stbi_load(fullFilePath,(i32*)&tempTexture.width,(i32*)&tempTexture.height,(i32*)&tempTexture.channelCount,requiredChannelCount);
    tempTexture.channelCount = requiredChannelCount;

    if(data){
        u32 currentGeneration = texture->generation;
        texture->generation = INVALID_ID;
        u64 totalSize = tempTexture.width * tempTexture.height * requiredChannelCount;
        // check for transparency
        b32 hasTransparency = false;
        for(u64 i=0; i < totalSize; i+=requiredChannelCount ){
            u8 a = data[i + 3];
            if(a < 255){
                hasTransparency = true;
                break;
            }
        }
        if(stbi_failure_reason()){
            KWARN("load_texture() failed to load file  '%s' : '%s' ",fullFilePath,stbi_failure_reason());
            stbi__err(0,0);
            return false;
        }

        // Take a copy of the name.
        string_ncopy(tempTexture.name, textureName, TEXTURE_NAME_MAX_LENGTH);
        tempTexture.generation = INVALID_ID;
        tempTexture.hasTransparency = hasTransparency;


        renderer_create_texture( data, &tempTexture);
        // Take a copy of the old texture
        Texture oldTexture = *texture;
        // Assign the temp texture to the pointer
        *texture = tempTexture;

        // destroy the old texture
        renderer_destroy_texture(&oldTexture);

        if(currentGeneration == INVALID_ID){
            texture->generation = 0;
        }
        else{
            texture->generation = currentGeneration + 1;
        }

        // clean up data
        stbi_image_free(data);
        return true;


    }
    else{
        if(stbi_failure_reason()){
            KWARN("load_texture() failed to load file  '%s' : '%s' ",fullFilePath,stbi_failure_reason());
            stbi__err(0,0);
        }
        return false;
    }
}

void destroy_texture(Texture* texture){

    renderer_destroy_texture(texture);
    kzero_memory(texture->name,sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
    kzero_memory(texture,sizeof(Texture));
    texture->id = INVALID_ID;
    texture->generation = INVALID_ID;
}