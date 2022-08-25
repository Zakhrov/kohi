#include "renderer/renderer_backend.h"
#include "renderer/vulkan_backend/vulkan_backend.h"
b8 renderer_backend_create(RendererBackendType rendererBackendType,RendererBackend* backend){
    
    if(rendererBackendType == RENDERER_BACKEND_TYPE_VULKAN){
        backend->initialize = vulkan_renderer_backend_initialize;
        backend->shutdown = vulkan_renderer_backend_shutdown;
        backend->begin_frame = vulkan_renderer_backend_begin_frame;
        backend->end_frame = vulkan_renderer_backend_end_frame;
        backend->resized = vulkan_renderer_backend_on_resized;

        

        return true;
    }
    return false;
    

}
void renderer_backend_destroy(RendererBackend* backend){
    backend->initialize = 0;
    backend->begin_frame = 0;
    backend->end_frame = 0;
    backend->resized = 0;
    backend->shutdown = 0;

}