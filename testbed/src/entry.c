#include <entry.h>
#include "game.h"
#include <memory/kmemory.h>

// Creates the game
b8 create_game(Game* outGame){
    
    outGame->applicationConfig.startX = 100;
    outGame->applicationConfig.startY = 100;
    outGame->applicationConfig.name = (char*)"Kohi Testbed";
    outGame->applicationConfig.startWidth = 640;
    outGame->applicationConfig.startHeight = 480;
    outGame->initialize = game_initialize;
    outGame->update = game_update;
    outGame->render = game_render;
    outGame->on_resize = game_on_resize;
    outGame->applicationState = 0;
    //Create Game State
    outGame->state = kallocate(sizeof(GameState),MEMORY_TAG_GAME);
    

    return true;
    

    
}
