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

void renderer_create_texture(const u8* pixels, Texture* texture);

void renderer_destroy_texture(Texture* texture);

b8 renderer_create_material(Material* material);

void renderer_destroy_material(Material* material);

b8 renderer_create_geometry(Geometry* geometry,u32 vertexCount,const Vertex3D* vertices, u32 indexCount,const u32* indices);
void renderer_destroy_geometry(Geometry* geometry);


#ifdef __cplusplus
}
#endif
