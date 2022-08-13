#include "application.h"
#include "../platform/platform.h"
#include "../vulkan_test.h"
typedef struct ApplicationState{
    b8 isRunning;
    b8 isSuspended;
    PlatformState platform;
    i16 width;
    i16 height;
    f64 lastTime;

}AppicationState;

static b8 initialized = FALSE;
static ApplicationState applicationState{};

b8 application_create(ApplicationConfig* config){
    if(initialized){
        KERROR("Application Create called more than once");
        return FALSE;
    }
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

    if (!platform_startup(&applicationState.platform,config->name,config->startX,config->startY,config->startWidth,config->startHeight)) {
        return FALSE;
    }
    vulkan_test_init();
    
    initialized = TRUE;
    return TRUE;


}

b8 application_run(){
    while (applicationState.isRunning)
    {
        if(!platform_pump_messages(&applicationState.platform)){
            applicationState.isRunning = FALSE;
        }
    }
    applicationState.isRunning = FALSE;
    platform_shutdown(&applicationState.platform);
    vulkan_test_shutdown();


    return TRUE;
    

}