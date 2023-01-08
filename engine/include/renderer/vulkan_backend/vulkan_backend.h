#pragma once
#include "../renderer_backend.h"
#ifdef __cplusplus
#include "vulkan_types.inl"
extern "C"
{
#endif
b8 vulkan_renderer_backend_initialize(RendererBackend* backend, const char* applicationName);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

void vulkan_renderer_backend_on_resized(RendererBackend* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 deltaTime);
void vulkan_renderer_backend_update_global_state(RendererBackend* backend,mat4 projection,mat4 view, vec3 viewPosition,vec4 ambientColor,i32 mode);
b8 vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 deltaTime); 
void vulkan_renderer_backend_draw_geometry(RendererBackend* backend, GeometryRenderData data);
void vulkan_renderer_backend_create_texture(const u8* pixels, Texture* texture);
void vulkan_renderer_backend_destroy_texture(Texture* texture);
b8 vulkan_renderer_backend_create_material(Material* material);
void vulkan_renderer_backend_destroy_material(Material* material);
b8 vulkan_renderer_backend_create_geometry(Geometry* geometry,u32 vertexCount,const Vertex3D* vertices, u32 indexCount,const u32* indices);
void vulkan_renderer_backend_destroy_geometry(Geometry* geometry);
#ifdef __cplusplus
}
void vulkan_renderer_backend_create_texture_for_device(VulkanBuffer* stagingBuffers,const u8* pixels, struct VulkanTexture* texture, int deviceIndex);
void vulkan_renderer_backend_destroy_texture_for_device(VulkanTextureData* data,int deviceIndex);
#endif


