#pragma once

#include "../renderer/renderer_types.inl"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct TextureSystemConfig{
    u32 maxTextureCount;
}TextureSystemConfig;

#define DEFAULT_TEXTURE_NAME "default"

b8 texture_system_initialize(u64* memoryRequirement, void* state, TextureSystemConfig config);

void texture_system_shutdown(void* state);

Texture* texture_system_acquire(const char* name, b8 autoRelease);
void texture_system_release(const char* name);


Texture* texture_system_get_default_texture();
#ifdef __cplusplus
}
#endif

