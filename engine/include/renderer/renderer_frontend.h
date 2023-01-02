#pragma once

#include "renderer_types.inl"

struct StaticMeshData;


#ifdef __cplusplus
extern "C"
{
#endif

b8 renderer_system_initialize(u64* memoryRequirement, void* state,void* platformState,const char* applicationName);
void renderer_shutdown();
void renderer_on_resized(u16 width, u16 height);
b8 renderer_draw_frame(RenderPacket* packet);

// HACK: This should not be exposed outside the engine!!!!
KAPI void renderer_set_view(mat4 view);

void renderer_create_texture(const char* name, b8 autoRelease, i32 width, i32 height, i32 channelCount, const u8* pixels, b8 hasTransparency, Texture* texture);

void renderer_destroy_texture(Texture* texture);


#ifdef __cplusplus
}
#endif
