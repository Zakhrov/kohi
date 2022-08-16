#include "renderer_frontend.h"
#include "renderer_backend.h"

#include "../core/logger.h"
#include "../core/kmemory.h"

static RendererBackend* backend = 0;

b8 renderer_initialize(const char* applicationName,struct PlatformState* platformState){
    backend = kallocate(sizeof(RendererBackend),MEMORY_TAG_RENDERER);

    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN,platformState,backend);
    backend->frameNumber = 0;
    if(!backend->initialize(backend,applicationName,platformState)){
        KFATAL("Renderer Backend failed to initialize Shutting down");
        return FALSE;

    }
    return TRUE;


}
void renderer_shutdown(){
    backend->shutdown(backend);
    kfree(backend,sizeof(RendererBackend),MEMORY_TAG_RENDERER);

}
void renderer_on_resized(u16 width, u16 height){
    backend->resized(backend,width,height);

}
b8 renderer_begin_frame(f32 deltaTime){
    return backend->begin_frame(backend,deltaTime);

}
b8 renderer_end_frame(f32 deltaTime){
    b8 result = backend->end_frame(backend,deltaTime);
    backend->frameNumber++;
    return result;
}

b8 renderer_draw_frame(RenderPacket* packet){

    if(renderer_begin_frame(packet->deltaTime)){
        b8 result = renderer_end_frame(packet->deltaTime);
        if(!result){
            KERROR("renderer_end_frame failed. Application shutting down......");
            return FALSE;
        }
    }
    return TRUE;

}
