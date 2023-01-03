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
void vulkan_renderer_backend_update_object(RendererBackend* backend, GeometryRenderData data);
void vulkan_renderer_backend_create_texture(RendererBackend* backend,const char* name, i32 width, i32 height, i32 channelCount, const u8* pixels, b8 hasTransparency, Texture* texture);
void vulkan_renderer_backend_destroy_texture(RendererBackend* backend,Texture* texture);
#ifdef __cplusplus
}
void vulkan_renderer_backend_create_texture_for_device(VulkanBuffer* stagingBuffers,const char* name, i32 width, i32 height, i32 channelCount, const u8* pixels, b8 hasTransparency, struct VulkanTextureData* data, int deviceIndex);
void vulkan_renderer_backend_destroy_texture_for_device(VulkanTextureData* data,int deviceIndex);
#endif


