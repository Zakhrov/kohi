#include "renderer/renderer_backend.h"
#include "renderer/vulkan_backend/vulkan_backend.h"
b8 renderer_backend_create(RendererBackendType rendererBackendType,RendererBackend* backend){
    
    if(rendererBackendType == RENDERER_BACKEND_TYPE_VULKAN){
        backend->initialize = vulkan_renderer_backend_initialize;
        backend->shutdown = vulkan_renderer_backend_shutdown;
        backend->begin_frame = vulkan_renderer_backend_begin_frame;
        backend->update_global_state = vulkan_renderer_backend_update_global_state;
        backend->update_object = vulkan_renderer_backend_update_object;
        backend->end_frame = vulkan_renderer_backend_end_frame;
        backend->resized = vulkan_renderer_backend_on_resized;
        backend->create_texture = vulkan_renderer_backend_create_texture;
        backend->destroy_texture = vulkan_renderer_backend_destroy_texture;
        backend->create_material = vulkan_renderer_backend_create_material;
        backend->destroy_material = vulkan_renderer_backend_destroy_material;
        

        

        return true;
    }
    return false;
    

}
void renderer_backend_destroy(RendererBackend* backend){
    backend->initialize = 0;
    backend->begin_frame = 0;
    backend->update_global_state = 0;
    backend->update_object = 0;
    backend->end_frame = 0;
    backend->resized = 0;
    backend->shutdown = 0;
    backend->create_texture = 0;
    backend->destroy_texture = 0;
    backend->create_material = 0;
    backend->destroy_material = 0;
    

}