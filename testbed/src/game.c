#include "game.h"
#include <core/logger.h>


b8 game_initialize(Game* gameInstance){
    KDEBUG("Game Initialized called");
    return TRUE;

}

b8 game_update(Game* gameInstance,f32 deltaTime){
    // KDEBUG("Game Update called");
    return TRUE;
}

b8 game_render(Game* gameInstance,f32 deltaTime){
    // KDEBUG("Game Render called");
    return TRUE;
}

void game_on_resize(Game* gameInstance,u32 width, u32 height){
    KDEBUG("Game On Resize called");
}

