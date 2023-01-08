#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

#include "core/logger.h"
#include "memory/kmemory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"



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


    
    
    // TODO: make this configurable
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN,&statePtr->backend);
    statePtr->backend.frameNumber = 0;
    statePtr->backend.platformState = platformState;
    if(!statePtr->backend.initialize(&statePtr->backend,applicationName)){
        KFATAL("Renderer Backend failed to initialize Shutting down");
        return false;

    }
    statePtr->nearClip = 0.1f;
    statePtr->farClip = 1000.0f;

    statePtr->projection = mat4_perspective(deg_to_rad(45.0f),640/480.0f,statePtr->nearClip,statePtr->farClip);
    statePtr->view = mat4_translation((vec3){0, 0, -30.0f});
    statePtr->view = mat4_inverse(statePtr->view);
    KINFO("Renderer Subsystem Initialized");
    return true;
    


}
void renderer_shutdown(){
    if(statePtr){

        statePtr->backend.shutdown(&statePtr->backend);
        
    }
    statePtr = 0;
    

}
void renderer_on_resized(u16 width, u16 height){
    statePtr->projection = mat4_perspective(deg_to_rad(45.0f),width / (f32)height,statePtr->nearClip,statePtr->farClip);
    statePtr->backend.resized(&statePtr->backend,width,height);

}
b8 renderer_begin_frame(f32 deltaTime){
    if(!statePtr){
        return false;
    }
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
        u32 count = packet->geometryCount;
        for(u32 i=0; i < count; i++){
            statePtr->backend.draw_geometry(&statePtr->backend,packet->geometries[i]);
        }
        b8 result = renderer_end_frame(packet->deltaTime);
        if(!result){
            KERROR("renderer_end_frame failed. Application shutting down......");
            return false;
        }
    }
    return true;

}
void renderer_set_view(mat4 view){
    statePtr->view = view;
}

void renderer_create_texture(const u8* pixels, Texture* texture){
    statePtr->backend.create_texture(pixels,texture);
}


void renderer_destroy_texture(Texture* texture){
    statePtr->backend.destroy_texture(texture);
}

b8 renderer_create_material(Material* material){
    KDEBUG("MATERIAL STUB %d",material->diffuseMap.texture->internalData);
    return statePtr->backend.create_material(material);

}

void renderer_destroy_material(Material* material){
    KDEBUG("MATERIAL STUB %d",material->diffuseMap.texture->internalData);
    statePtr->backend.destroy_material(material);

}

b8 renderer_create_geometry(Geometry* geometry,u32 vertexCount,const Vertex3D* vertices, u32 indexCount,const u32* indices){
    return statePtr->backend.create_geometry(geometry,vertexCount,vertices,indexCount,indices);
}
void renderer_destroy_geometry(Geometry* geometry){
    statePtr->backend.destroy_geometry(geometry);
}
