#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

#include "core/logger.h"
#include "memory/kmemory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"

// TODO: Temporary
#include "core/kstring.h"
#include "core/event.h"
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: End Temporary

typedef struct RendererSystemState{
    RendererBackend backend;
    mat4 projection;
    f32 nearClip;
    f32 farClip;
    mat4 view;

    Texture defaultTexture;
    // TODO: Temporary
    Texture testDiffuse;
    // TODO: End Temporary

}RendererSystemState;

static RendererSystemState* statePtr;

void create_texture(Texture* texture){
    kzero_memory(texture,sizeof(Texture));
    texture->generation = INVALID_ID;
}
b8 load_texture(const char* textureName,Texture* texture){
    // TODO: Should be able to be located anywhere
    char* formatStr = "../assets/textures/%s.%s";
    const i32 requiredChannelCount = 4;
    stbi_set_flip_vertically_on_load(true);
    char fullFilePath[512];
    // TODO: try different extensions
    string_format(fullFilePath,formatStr,textureName,"png");

    // Use a temporary local texture to load everything
    Texture tempTexture;
    u8* data = stbi_load(fullFilePath,(i32*)&tempTexture.width,(i32*)&tempTexture.height,(i32*)&tempTexture.channelCount,requiredChannelCount);
    tempTexture.channelCount = requiredChannelCount;

    if(data){
        u32 currentGeneration = texture->generation;
        texture->generation = INVALID_ID;
        u64 totalSize = tempTexture.width * tempTexture.height * requiredChannelCount;
        // check for transparency
        b32 hasTransparency = false;
        for(u64 i=0; i < totalSize; i+=requiredChannelCount ){
            u8 a = data[i + 3];
            if(a < 255){
                hasTransparency = true;
                break;
            }
        }
        if(stbi_failure_reason()){
            KWARN("load_texture() failed to load file  '%s' : '%s' ",fullFilePath,stbi_failure_reason());
        }
        renderer_create_texture( textureName, true, tempTexture.width, tempTexture.height, requiredChannelCount, data, hasTransparency, &tempTexture);
        // Take a copy of the old texture
        Texture oldTexture = *texture;
        // Assign the temp texture to the pointer
        *texture = tempTexture;

        // destroy the old texture
        renderer_destroy_texture(&oldTexture);

        if(currentGeneration == INVALID_ID){
            texture->generation = 0;
        }
        else{
            texture->generation = currentGeneration + 1;
        }

        // clean up data
        stbi_image_free(data);
        return true;


    }
    else{
        if(stbi_failure_reason()){
            KWARN("load_texture() failed to load file  '%s' : '%s' ",fullFilePath,stbi_failure_reason());
        }
        return false;
    }
}

// TODO: Temp
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, EventContext data) {
    const char* names[3] = {
        "Brick_01",
        "Brick_02",
        "girl1"};
    static i8 choice = 2;
    choice++;
    choice %= 3;

    // Load up the new texture.
    load_texture(names[choice], &statePtr->testDiffuse);
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

    // Take a pointer to default textures for use in the backend
    statePtr->backend.defaultDiffuse = &statePtr->defaultTexture;
    
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

    // NOTE: Create default texture, a 256x256 blue/white checkerboard pattern.
    // This is done in code to eliminate asset dependencies.
    KTRACE("Creating default texture...");
    const u32 tex_dimension = 256;
    const u32 channels = 4;
    const u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[pixel_count * channels];
    //u8* pixels = kallocate(sizeof(u8) * pixel_count * bpp, MEMORY_TAG_TEXTURE);
    kset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);

    // Each pixel.
    for (u64 row = 0; row < tex_dimension; ++row) {
        for (u64 col = 0; col < tex_dimension; ++col) {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }
    renderer_create_texture( "default", false, tex_dimension, tex_dimension, 4, pixels, false, &statePtr->defaultTexture);
    // Manually set default texture generation to INVALID_ID since this is the default texture
    statePtr->defaultTexture.generation = INVALID_ID;

    // TODO: Load other textures
    create_texture(&statePtr->testDiffuse);
    





    return true;
    


}
void renderer_shutdown(){
    if(statePtr){
         // TODO: temp
        event_unregister(EVENT_CODE_DEBUG0, statePtr, event_on_debug_event);
        // TODO: end temp
        renderer_destroy_texture(&statePtr->defaultTexture);
        renderer_destroy_texture(&statePtr->testDiffuse);
        statePtr->backend.shutdown(&statePtr->backend);
        // kfree(&statePtr->backend,sizeof(RendererBackend),MEMORY_TAG_RENDERER);
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
        data.objectId = 0; // TODO: Actual Object 
        data.textures[0] = &statePtr->testDiffuse;
        statePtr->backend.update_object(&statePtr->backend,data);
        angle += 0.005f;
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

void renderer_create_texture(const char* name, b8 autoRelease, i32 width, i32 height, i32 channelCount, const u8* pixels, b8 hasTransparency, Texture* texture){
    statePtr->backend.create_texture(&statePtr->backend, name,autoRelease,width,height,channelCount,pixels,hasTransparency,texture);
}


void renderer_destroy_texture(Texture* texture){
    statePtr->backend.destroy_texture(&statePtr->backend,texture);
}
