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
void renderer_set_view(mat4 view);

b8 renderer_draw_frame(RenderPacket* packet);


#ifdef __cplusplus
}
#endif
