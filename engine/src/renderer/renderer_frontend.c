#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

#include "core/logger.h"
#include "memory/kmemory.h"
#include "math/kmath.h"
typedef struct RendererSystemState{
    RendererBackend backend;
    mat4 projection;
    f32 nearClip;
    f32 farClip;
    mat4 view;

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
    statePtr->nearClip = 0.1f;
    statePtr->farClip = 1000.0f;

    statePtr->projection = mat4_perspective(deg_to_rad(45.0f),1280/720.0f,statePtr->nearClip,statePtr->farClip);
    statePtr->view = mat4_translation((vec3){0, 0, -30.0f});
    statePtr->view = mat4_inverse(statePtr->view);


    return true;
    


}
void renderer_shutdown(){
    statePtr->backend.shutdown(&statePtr->backend);
    // kfree(&statePtr->backend,sizeof(RendererBackend),MEMORY_TAG_RENDERER);

}
void renderer_on_resized(u16 width, u16 height){
    statePtr->projection = mat4_perspective(deg_to_rad(45.0f),width / (f32)height,statePtr->nearClip,statePtr->farClip);
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
        
        statePtr->backend.update_global_state(&statePtr->backend,statePtr->projection,statePtr->view,vec3_zero(),vec4_one(),0);
        static f32 angle = 0.01f;
        quat rotation = quat_from_axis_angle(vec3_forward(),angle,false);
        mat4 model = quat_to_rotation_matrix(rotation,vec3_zero());
        statePtr->backend.update_object(&statePtr->backend,model);
        angle += 0.001f;
        b8 result = renderer_end_frame(packet->deltaTime);
        if(!result){
            KERROR("renderer_end_frame failed. Application shutting down......");
            return false;
        }
    }
    return true;

}
// HACK: This should not be exposed outside the engine!!!!
KAPI void renderer_set_view(mat4 view){
    statePtr->view = view;
}
