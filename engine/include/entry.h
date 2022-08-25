#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "game_types.h"
#include "memory/kmemory.h"

// Externally defined function to create a game
extern b8 create_game(Game* outGame);

/****************************
    New Game ENtry point 

******************************/
int main(void){

    


    Game gameInstance;
    if(!create_game(&gameInstance)){
        KFATAL("Could not Create Game");
        return -1;
    }

    if(!gameInstance.render || !gameInstance.initialize || !gameInstance.on_resize || !gameInstance.update ){
        KFATAL("Game function pointers must be assigned");
        return -2;
    }

    

    if(!application_create(&gameInstance)){
        KFATAL("Failed to create Application");
        return 1;
    }
    // Begin game loop
    if(!application_run()){
        KINFO("Application did not shut down gracefully");
        return 2;

    }
    
    
    
    return 0;
}