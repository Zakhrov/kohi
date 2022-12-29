#pragma once
#include "../renderer_backend.h"
#ifdef __cplusplus
extern "C"
{
#endif
b8 vulkan_renderer_backend_initialize(RendererBackend* backend, const char* applicationName);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

void vulkan_renderer_backend_on_resized(RendererBackend* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 deltaTime);
void vulkan_renderer_backend_update_global_state(RendererBackend* backend,mat4 projection,mat4 view, vec3 viewPosition,vec4 ambientColor,i32 mode);
b8 vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 deltaTime); 
void vulkan_renderer_backend_update_object(RendererBackend* backend, mat4 model);
#ifdef __cplusplus
}
#endif


