#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

#include "core/logger.h"
#include "memory/kmemory.h"

typedef struct RendererSystemState{
    RendererBackend backend;
}RendererSystemState;

static RendererSystemState* statePtr;

b8 renderer_system_initialize(u64* memoryRequiremnt, void* state, void* platformState, const char* applicationName){
    *memoryRequiremnt = sizeof(RendererSystemState);
    if(state == 0){
        return false;
    }
    statePtr = state;
    

    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN,&statePtr->backend);
    statePtr->backend.frameNumber = 0;
    statePtr->backend.platformState = platformState;
    if(!statePtr->backend.initialize(&statePtr->backend,applicationName)){
        KFATAL("Renderer Backend failed to initialize Shutting down");
        return false;

    }

    return true;
    


}
void renderer_shutdown(){
    statePtr->backend.shutdown(&statePtr->backend);
    // kfree(&statePtr->backend,sizeof(RendererBackend),MEMORY_TAG_RENDERER);

}
void renderer_on_resized(u16 width, u16 height){
    statePtr->backend.resized(&statePtr->backend,width,height);

}
b8 renderer_begin_frame(f32 deltaTime){
    return statePtr->backend.begin_frame(&statePtr->backend,deltaTime);

}
b8 renderer_end_frame(f32 deltaTime){
    b8 result = statePtr->backend.end_frame(&statePtr->backend,deltaTime);
    statePtr->backend.frameNumber++;
    return result;
}

b8 renderer_draw_frame(RenderPacket* packet){

    if(renderer_begin_frame(packet->deltaTime)){
        b8 result = renderer_end_frame(packet->deltaTime);
        if(!result){
            KERROR("renderer_end_frame failed. Application shutting down......");
            return false;
        }
    }
    return true;

}
