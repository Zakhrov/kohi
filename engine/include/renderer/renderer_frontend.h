#pragma once

#include "renderer_types.inl"

struct StaticMeshData;
struct PlatformState;

#ifdef __cplusplus
extern "C"
{
#endif

b8 renderer_initialize(const char* applicationName,struct PlatformState* platformState);
void renderer_shutdown();
void renderer_on_resized(u16 width, u16 height);

b8 renderer_draw_frame(RenderPacket* packet);


#ifdef __cplusplus
}
#endif
