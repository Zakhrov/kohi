#include "core/application.h"
#include "platform/platform.h"

#include "game_types.h"
#include "memory/kmemory.h"
#include "core/clock.h"
#include "memory/linear_allocator.h"
#include "core/kstring.h"

// Renderer
#include "renderer/renderer_frontend.h"

// Systems
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"

//TODO: Begin Temp code
#include "math/kmath.h"
//TODO: End Temp Code


typedef struct ApplicationState{
    Game* gameInstance;
    b8 isRunning;
    b8 isSuspended;
    i16 width;
    i16 height;
    KohiClock clock;
    f64 lastTime;
    LinearAllocator systemsAllocator;

     u64 eventSystemMemoryReqs;
    void* eventSystemState;

    u64 inputSystemMemoryReqs;
    void* inputSystemState;

    u64 loggingSystemMemoryReqs;
    void* loggingSystemState;

    u64 memorySystemMemoryReqs;
    void* memorySystemState;

    u64 platformSystemMemoryReqs;
    void* platformSystemState;

    u64 rendererSystemMemoryReqs;
    void* rendererSystemState;

    u64 textureSystemMemoryReqs;
    void* textureSystemState;

    u64 materialSystemMemoryReqs;
    void* materialSystemState;

    u64 geometrySystemMemoryReqs;
    void* geometrySystemState;
    u64 resourceSystemMemoryReqs;
    void* resourceSystemState;

    //TODO: Temp
    Geometry* testGeometry;
    //TODO: End Temp


} ApplicationState;


static ApplicationState* applicationState;


b8 application_on_event(u16 code, void* sender, void* listener_inst, EventContext context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, EventContext context);
b8 application_on_resize(u16 code, void* sender, void* listener_inst, EventContext context);
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
    if(applicationState->testGeometry){
        applicationState->testGeometry->material->diffuseMap.texture = texture_system_acquire(names[choice], true);
        if (!applicationState->testGeometry->material->diffuseMap.texture) {
            KWARN("event_on_debug_event no texture! using default");
            applicationState->testGeometry->material->diffuseMap.texture = texture_system_get_default_texture();
        }
    }
    // Release the old texture
    texture_system_release(oldName);
    
    return true;
}
//TODO: End Temp

b8 application_create(Game* gameInstance){
    if(gameInstance->applicationState){
        KERROR("Application Create called more than once");
        return false;
    }
    // kfree(gameInstance->applicationState,sizeof(ApplicationState),MEMORY_TAG_APPLICATION);
    gameInstance->applicationState = kallocate(sizeof(ApplicationState),MEMORY_TAG_APPLICATION);
    applicationState = gameInstance->applicationState;
    applicationState->gameInstance = gameInstance;
    applicationState->isRunning = false;
    applicationState->isSuspended = false;
    u64 systemsAllocatorTotalSize = 64 * 1024 * 1024; // 64 MiB
    linear_allocator_create(systemsAllocatorTotalSize,0,&applicationState->systemsAllocator);
    

    

    
    // Initialize subsystems

    // Memory
    memory_system_initialize(&applicationState->memorySystemMemoryReqs,0);
    KDEBUG("MEMORY SYSTEM REQS %i",applicationState->memorySystemMemoryReqs);
    applicationState->memorySystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->memorySystemMemoryReqs);
    memory_system_initialize(&applicationState->memorySystemMemoryReqs,applicationState->memorySystemState);

    // Logging
    initialize_logging(&applicationState->loggingSystemMemoryReqs,0);
    applicationState->loggingSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->loggingSystemMemoryReqs);
    if(!initialize_logging(&applicationState->loggingSystemMemoryReqs,applicationState->loggingSystemState)){
        KERROR("Logging system failed to initialize");
        return false;
    }

     // Events
    event_system_initialize(&applicationState->eventSystemMemoryReqs,0);
    applicationState->eventSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->eventSystemMemoryReqs);
    event_system_initialize(&applicationState->eventSystemMemoryReqs,applicationState->eventSystemState);

    // Inputs
    input_system_initialize(&applicationState->inputSystemMemoryReqs,0);
    applicationState->inputSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->inputSystemMemoryReqs);
    input_system_initialize(&applicationState->inputSystemMemoryReqs,applicationState->inputSystemState);

    
    platform_system_startup(&applicationState->platformSystemMemoryReqs,0,0,0,0,0,0);
    applicationState->platformSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->platformSystemMemoryReqs);
    platform_system_startup(&applicationState->platformSystemMemoryReqs,
    applicationState->platformSystemState,
    gameInstance->applicationConfig.name,
    gameInstance->applicationConfig.startX,
    gameInstance->applicationConfig.startY,
    gameInstance->applicationConfig.startWidth,
    gameInstance->applicationConfig.startHeight);

    // Resource System
    ResourceSystemConfig resource_sys_config;
    resource_sys_config.assetBasePath = "../assets";
    resource_sys_config.maxLoaderCount = 32;
    resource_system_initialize(&applicationState->resourceSystemMemoryReqs,0,resource_sys_config);
    applicationState->resourceSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->resourceSystemMemoryReqs);
    if(!resource_system_initialize(&applicationState->resourceSystemMemoryReqs,&applicationState->resourceSystemState,resource_sys_config)){
        KFATAL("Failed to initialize renderer");
        return false;
    }

    //Renderer System
    renderer_system_initialize(&applicationState->rendererSystemMemoryReqs,0,0,0);
    applicationState->rendererSystemState = linear_allocator_allocate(&applicationState->systemsAllocator,applicationState->rendererSystemMemoryReqs);
    
    if(!renderer_system_initialize(&applicationState->rendererSystemMemoryReqs,applicationState->rendererSystemState,applicationState->platformSystemState,gameInstance->applicationConfig.name)){
        KFATAL("Failed to initialize renderer");
        return false;
    }

    // Texture system.
    TextureSystemConfig texture_sys_config;
    texture_sys_config.maxTextureCount = 65536;
    texture_system_initialize(&applicationState->textureSystemMemoryReqs, 0, texture_sys_config);
    applicationState->textureSystemState = linear_allocator_allocate(&applicationState->systemsAllocator, applicationState->textureSystemMemoryReqs);
    if (!texture_system_initialize(&applicationState->textureSystemMemoryReqs, applicationState->textureSystemState, texture_sys_config)) {
        KFATAL("Failed to initialize texture system. Application cannot continue.");
        return false;
    }

    // Material system.
    MaterialSystemConfig material_sys_config;
    material_sys_config.maxMaterialCount = 4096;
    material_system_initialize(&applicationState->materialSystemMemoryReqs, 0, material_sys_config);
    applicationState->materialSystemState = linear_allocator_allocate(&applicationState->systemsAllocator, applicationState->materialSystemMemoryReqs);
    if (!material_system_initialize(&applicationState->materialSystemMemoryReqs, applicationState->materialSystemState, material_sys_config)) {
        KFATAL("Failed to initialize material system. Application cannot continue.");
        return false;
    }

    // Geometry system.
    GeometrySystemConfig geometry_sys_config;
    geometry_sys_config.maxGeometryCount = 4096;
    geometry_system_initialize(&applicationState->geometrySystemMemoryReqs, 0, geometry_sys_config);
    applicationState->geometrySystemState = linear_allocator_allocate(&applicationState->systemsAllocator, applicationState->geometrySystemMemoryReqs);
    if (!geometry_system_initialize(&applicationState->geometrySystemMemoryReqs, applicationState->geometrySystemState, geometry_sys_config)) {
        KFATAL("Failed to initialize Geometry system. Application cannot continue.");
        return false;
    }
    // TODO: Temp
    GeometryConfig g_config = geometry_system_generate_plane_config(10.0f, 10.0f, 5, 5, 2.0f, 2.0f, "test geometry", "test_material");
    applicationState->testGeometry = geometry_system_acquire_from_config(g_config, true);

    // Clean up the allocations for the geometry config.
    kfree(g_config.vertices, sizeof(Vertex3D) * g_config.vertexCount, MEMORY_TAG_ARRAY);
    kfree(g_config.indices, sizeof(u32) * g_config.indexCount, MEMORY_TAG_ARRAY);
    
    // Load default Geometry
    // applicationState->testGeometry = geometry_system_get_default();
    // TODO: End Temp

    

    // Event Registration 
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED ,0,application_on_resize);
    // TODO: Temp
    event_register(EVENT_CODE_DEBUG0,0,event_on_debug_event);
    // TODO: End Temp
    // Initialize the Game
    if(!applicationState->gameInstance->initialize(applicationState->gameInstance)){
        KFATAL("Failed to initialize game");
        return false;
    }
    applicationState->gameInstance->on_resize(applicationState->gameInstance,applicationState->width,applicationState->height);

    
    
    
    return true;


}

b8 application_run(){
    applicationState->isRunning = true;
    clock_start(&applicationState->clock);
    clock_update(&applicationState->clock);
    applicationState->lastTime = applicationState->clock.elaplsedTime;
    f64 runTime = 0;
    u8 frameCount = 0;
    f64 targetFrameTimeSeconds = 1.0f / 60;
    
    KINFO(get_memory_usage_str());
    while (applicationState->isRunning)
    {
        if(!platform_pump_messages(&applicationState->platformSystemState)){
            applicationState->isRunning = false;
        }
        if(!applicationState->isSuspended){
            clock_update(&applicationState->clock);
            f64 currentTime = applicationState->clock.elaplsedTime;
            f64 deltaTime = currentTime - applicationState->lastTime;
            f64 frameStartTime = platform_get_absolute_time();

            if(!applicationState->gameInstance->update(applicationState->gameInstance,(f32)deltaTime)){
                KFATAL("Failed to update game state Shutting down");
                applicationState->isRunning = false;
                break;
            }

            if(!applicationState->gameInstance->render(applicationState->gameInstance,(f32)deltaTime)){
                KFATAL("Failed to Render game Shutting down");
                applicationState->isRunning = false;
                break;
            }
            // TODO: Refactor Packet creation
            RenderPacket packet;
            packet.deltaTime = deltaTime;
            // TODO: Temp
            GeometryRenderData testRender;
            testRender.geometry = applicationState->testGeometry;
            testRender.model = mat4_identity();
            packet.geometryCount = 1;
            packet.geometries = &testRender;

            // TODO: End Temp


            renderer_draw_frame(&packet);

            f64 frameEndTime = platform_get_absolute_time();
            f64 frameElapsedTime = frameEndTime - frameStartTime;
            runTime += frameElapsedTime;
            f64 remainingSeconds = targetFrameTimeSeconds - frameElapsedTime;
            if(remainingSeconds > 0){
                u64 remainingMs = remainingSeconds * 1000;
                b8 frameLimit = false;
                if(remainingMs > 0 && frameLimit){
                    platform_sleep(remainingMs -1);
                }
                frameCount++;
            }
            input_update(deltaTime);

            applicationState->lastTime = currentTime;
        }

        
    }
    applicationState->isRunning = false;
     event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    // TODO: Temp
    event_unregister(EVENT_CODE_DEBUG0,0,event_on_debug_event);
    // TODO: End Temp
    
    input_system_shutdown(applicationState->inputSystemState);

    geometry_system_shutdown(applicationState->geometrySystemState);
    material_system_shutdown(applicationState->materialSystemState);
    texture_system_shutdown(applicationState->textureSystemState);
    renderer_shutdown();
    resource_system_shutdown(applicationState->resourceSystemState);
    platform_system_shutdown(&applicationState->platformSystemState);
    memory_system_shutdown(applicationState->memorySystemState);
    
    


    return true;
    

}

void application_get_framebuffer_size(u64* width, u64* height){
    *width = applicationState->width;
    *height = applicationState->height;
    
}


b8 application_on_event(u16 code, void* sender, void* listener_inst, EventContext context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            applicationState->isRunning = false;
            return true;
        }
    }

    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, EventContext context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            EventContext data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%d' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            KDEBUG("Explicit - B key released!");
        } else {
            KDEBUG("'%d' key released in window.", key_code);
        }
    }
    return false;
} 

b8 application_on_resize(u16 code, void* sender, void* listener_inst, EventContext context){
    if(code == EVENT_CODE_RESIZED){
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];
        if(width != applicationState->width || height != applicationState->height){
            applicationState->width = width;
            applicationState->height = height;
            KDEBUG("Window resize %i, %i",width,height);
            if(width == 0 || height == 0){
                KINFO("Window minimized suspending application");
                applicationState->isSuspended = true;
                return true;
            }
            else{
                if(applicationState->isSuspended){
                    KINFO("Window restored resuming application");
                    applicationState->isSuspended = false;

                }
                applicationState->gameInstance->on_resize(applicationState->gameInstance,width,height);
                renderer_on_resized(width, height);
            }
        }
    }
    return false; //  explicitly not handle so other events can fire
}