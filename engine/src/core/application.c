#include "application.h"
#include "../platform/platform.h"

#include "../game_types.h"
#include "kmemory.h"
#include "clock.h"
#include "../renderer/renderer_frontend.h"


typedef struct ApplicationState{
    Game* gameInstance;
    b8 isRunning;
    b8 isSuspended;
    PlatformState platform;
    i16 width;
    i16 height;
    KohiClock clock;
    f64 lastTime;

} ApplicationState;

static b8 initialized = FALSE;
static ApplicationState applicationState;

b8 application_on_event(u16 code, void* sender, void* listener_inst, EventContext context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, EventContext context);

b8 application_create(Game* gameInstance){
    if(initialized){
        KERROR("Application Create called more than once");
        return FALSE;
    }
    applicationState.gameInstance = gameInstance;
    // Initialize subsystems
    initialize_logging();
    


    applicationState.isRunning = TRUE;
    applicationState.isSuspended = FALSE;

      if(!event_initialize()) {
        KERROR("Event system failed initialization. Application cannot continue.");
        return FALSE;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    if (!platform_startup(&applicationState.platform,
    gameInstance->applicationConfig.name,
    gameInstance->applicationConfig.startX,
    gameInstance->applicationConfig.startY,
    gameInstance->applicationConfig.startWidth,
    gameInstance->applicationConfig.startHeight)) {
        return FALSE;
    }
    if(!renderer_initialize(gameInstance->applicationConfig.name,&applicationState.platform)){
        KFATAL("Renderer initialization failed Shutting down...");
        return FALSE;
    }
    // Initialize the Game
    if(!applicationState.gameInstance->initialize(applicationState.gameInstance)){
        KFATAL("Failed to initialize game");
        return FALSE;
    }
    applicationState.gameInstance->on_resize(applicationState.gameInstance,applicationState.width,applicationState.height);

    
    
    initialized = TRUE;
    return TRUE;


}

b8 application_run(){
    clock_start(&applicationState.clock);
    clock_update(&applicationState.clock);
    applicationState.lastTime = applicationState.clock.elaplsedTime;
    f64 runTime = 0;
    u8 frameCount = 0;
    f64 targetFrameTimeSeconds = 1.0f / 60;
    
    KINFO(get_memory_usage_str());
    while (applicationState.isRunning)
    {
        if(!platform_pump_messages(&applicationState.platform)){
            applicationState.isRunning = FALSE;
        }
        if(!applicationState.isSuspended){
            clock_update(&applicationState.clock);
            f64 currentTime = applicationState.clock.elaplsedTime;
            f64 deltaTime = currentTime - applicationState.lastTime;
            f64 frameStartTime = platform_get_absolute_time();

            if(!applicationState.gameInstance->update(applicationState.gameInstance,(f32)deltaTime)){
                KFATAL("Failed to update game state Shutting down");
                applicationState.isRunning = FALSE;
                break;
            }

            if(!applicationState.gameInstance->render(applicationState.gameInstance,(f32)deltaTime)){
                KFATAL("Failed to Render game Shutting down");
                applicationState.isRunning = FALSE;
                break;
            }
            // TODO: Replace with something better
            RenderPacket packet;
            packet.deltaTime = deltaTime;
            renderer_draw_frame(&packet);

            f64 frameEndTime = platform_get_absolute_time();
            f64 frameElapsedTime = frameEndTime - frameStartTime;
            runTime += frameElapsedTime;
            f64 remainingSeconds = targetFrameTimeSeconds - frameElapsedTime;
            if(remainingSeconds > 0){
                u64 remainingMs = remainingSeconds * 1000;
                b8 frameLimit = FALSE;
                if(remainingMs > 0 && frameLimit){
                    platform_sleep(remainingMs -1);
                }
                frameCount++;
            }
            input_update(deltaTime);

            applicationState.lastTime = currentTime;
        }

        
    }
    applicationState.isRunning = FALSE;
     event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_shutdown();
    input_shutdown();
    
    renderer_shutdown();
    platform_shutdown(&applicationState.platform);
    


    return TRUE;
    

}

b8 application_on_event(u16 code, void* sender, void* listener_inst, EventContext context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            applicationState.isRunning = FALSE;
            return TRUE;
        }
    }

    return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, EventContext context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            EventContext data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return TRUE;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            KDEBUG("Explicit - B key released!");
        } else {
            KDEBUG("'%c' key released in window.", key_code);
        }
    }
    return FALSE;
} 