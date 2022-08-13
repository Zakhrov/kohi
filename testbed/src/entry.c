#include <entry.h>
#include "game.h"
#include <core/kmemory.h>

// Creates the game
b8 create_game(Game* outGame){
    
    outGame->applicationConfig.startX = 100;
    outGame->applicationConfig.startY = 100;
    outGame->applicationConfig.name = (char*)"Kohi Testbed";
    outGame->applicationConfig.startWidth = 1280;
    outGame->applicationConfig.startHeight = 720;
    outGame->initialize = game_initialize;
    outGame->update = game_update;
    outGame->render = game_render;
    outGame->on_resize = game_on_resize;
    //Create Game State
    outGame->state = kallocate(sizeof(GameState),MEMORY_TAG_GAME);

    return TRUE;
    

    
}