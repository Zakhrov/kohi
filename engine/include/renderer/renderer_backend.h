#pragma once
#include "renderer_types.inl"

struct PlatformState;

#ifdef __cplusplus
extern "C"
{
#endif

b8 renderer_backend_create(RendererBackendType rendererBackendType,RendererBackend* backend);
void renderer_backend_destroy(RendererBackend* backend);


#ifdef __cplusplus
}
#endif
