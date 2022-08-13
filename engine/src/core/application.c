#include "application.h"
#include "../platform/platform.h"
#include "../vulkan_test.h"
#include "../game_types.h"
#include "kmemory.h"


typedef struct ApplicationState{
    Game* gameInstance;
    b8 isRunning;
    b8 isSuspended;
    PlatformState platform;
    i16 width;
    i16 height;
    f64 lastTime;

} ApplicationState;

static b8 initialized = FALSE;
static ApplicationState applicationState;

b8 application_create(Game* gameInstance){
    if(initialized){
        KERROR("Application Create called more than once");
        return FALSE;
    }
    applicationState.gameInstance = gameInstance;
    // Initialize subsystems
    initialize_logging();
    KFATAL("A test message: %f", 3.14f);
    KERROR("A test message: %f", 3.14f);
    KWARN("A test message: %f", 3.14f);
    KINFO("A test message: %f", 3.14f);
    KDEBUG("A test message: %f", 3.14f);
    KTRACE("A test message: %f", 3.14f);

    applicationState.isRunning = TRUE;
    applicationState.isSuspended = FALSE;

    if (!platform_startup(&applicationState.platform,
    gameInstance->applicationConfig.name,
    gameInstance->applicationConfig.startX,
    gameInstance->applicationConfig.startY,
    gameInstance->applicationConfig.startWidth,
    gameInstance->applicationConfig.startHeight)) {
        return FALSE;
    }
    // Initialize the Game
    if(!applicationState.gameInstance->initialize(applicationState.gameInstance)){
        KFATAL("Failed to initialize game");
        return FALSE;
    }
    applicationState.gameInstance->on_resize(applicationState.gameInstance,applicationState.width,applicationState.height);

    vulkan_test_init();
    
    initialized = TRUE;
    return TRUE;


}

b8 application_run(){
    
    KINFO(get_memory_usage_str());
    while (applicationState.isRunning)
    {
        if(!platform_pump_messages(&applicationState.platform)){
            applicationState.isRunning = FALSE;
        }
        if(!applicationState.isSuspended){

            if(!applicationState.gameInstance->update(applicationState.gameInstance,(f32)0)){
                KFATAL("Failed to update game state Shutting down");
                applicationState.isRunning = FALSE;
                break;
            }

            if(!applicationState.gameInstance->render(applicationState.gameInstance,(f32)0)){
                KFATAL("Failed to Render game Shutting down");
                applicationState.isRunning = FALSE;
                break;
            }
        }

        
    }
    applicationState.isRunning = FALSE;
    platform_shutdown(&applicationState.platform);
    vulkan_test_shutdown();


    return TRUE;
    

}