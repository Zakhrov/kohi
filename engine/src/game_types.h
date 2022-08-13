#pragma once

#include "core/application.h"

typedef struct Game{
    ApplicationConfig applicationConfig;
    // Function pointer to initialize
    b8 (*initialize) (struct Game* gameInstance);
    // Function pointer to update
    b8 (*update) (struct Game* gameInstance, f32 deltaTime);
    // Function pointer to render
    b8 (*render) (struct Game* gameInstance,f32 deltaTime);
    // Function pointer to on_resize
    void (*on_resize) (struct Game* gameInstance,u32 width, u32 height);
    // Game specific Game state created and managed by the game
    void* state;

}Game;