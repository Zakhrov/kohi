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
b8 vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 deltaTime); 
#ifdef __cplusplus
}
#endif


