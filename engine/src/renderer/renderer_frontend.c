#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

#include "core/logger.h"
#include "memory/kmemory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

// TODO: Temporary
#include "core/kstring.h"
#include "core/event.h"

// TODO: End Temporary

typedef struct RendererSystemState{
    RendererBackend backend;
    mat4 projection;
    f32 nearClip;
    f32 farClip;
    mat4 view;
    Material* testMaterial;

}RendererSystemState;

static RendererSystemState* statePtr;



// TODO: Temp
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, EventContext data) {
    const char* names[3] = {
        "Brick_01",
        "Brick_02",
        "girl1"};
    static i8 choice = 2;
    // Save off old name
    const char* oldName = names[choice];
    choice++;
    choice %= 3;
    
    
    // Acquire the new texture.
    statePtr->testMaterial->diffuseMap.texture = texture_system_acquire(names[choice], true);
    if (!statePtr->testMaterial->diffuseMap.texture) {
        KWARN("event_on_debug_event no texture! using default");
        statePtr->testMaterial->diffuseMap.texture = texture_system_get_default_texture();
    }
    // Release the old texture
    texture_system_release(oldName);
    
    return true;
}
// TODO: End Temp


b8 renderer_system_initialize(u64* memoryRequiremnt, void* state, void* platformState, const char* applicationName){
    *memoryRequiremnt = sizeof(RendererSystemState);
    if(state == 0){
        return false;
    }
    statePtr = state;

     // TODO: temp
    event_register(EVENT_CODE_DEBUG0, statePtr, event_on_debug_event);
    // TODO: end temp

    
    
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
         // TODO: temp
        event_unregister(EVENT_CODE_DEBUG0, statePtr, event_on_debug_event);
        // TODO: end temp
        
        statePtr->backend.shutdown(&statePtr->backend);
        
    }
    statePtr = 0;
    

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
        GeometryRenderData data = {};
        data.model = model;
        data.material = statePtr->testMaterial;
        //TODO: Temporary
        
        // Create a default material if does not exist.
        if (!statePtr->testMaterial) {
            // Automatic config
            statePtr->testMaterial = material_system_acquire("test_material");
            if (!statePtr->testMaterial) {
                KWARN("Automatic material load failed, falling back to manual default material.");
                // Manual config
                MaterialConfig config;
                string_ncopy(config.name, "test_material", MATERIAL_NAME_MAX_LENGTH);
                config.autoRelease = false;
                config.diffuseColour = vec4_one();  // white
                string_ncopy(config.diffuseMapName, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
                statePtr->testMaterial = material_system_acquire_from_config(config);
            }
        }

        data.material = statePtr->testMaterial;

        statePtr->backend.update_object(&statePtr->backend,data);
        angle += 0.0005f;
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
    statePtr->backend.create_texture(&statePtr->backend, pixels,texture);
}


void renderer_destroy_texture(Texture* texture){
    statePtr->backend.destroy_texture(&statePtr->backend,texture);
}

b8 renderer_create_material(Material* material){
    KDEBUG("MATERIAL STUB %d",material->diffuseMap.texture->internalData);
    return statePtr->backend.create_material(material);

}

void renderer_destroy_material(Material* material){
    KDEBUG("MATERIAL STUB %d",material->diffuseMap.texture->internalData);
    statePtr->backend.destroy_material(material);

}
