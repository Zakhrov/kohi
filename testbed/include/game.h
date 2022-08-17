#pragma once
#include <defines.h>
#include <game_types.h>

typedef struct GameState{
    f32 deltaTime;
}GameState;

b8 game_initialize(Game* gameInstance);

b8 game_update(Game* gameInstance,f32 deltaTime);

b8 game_render(Game* gameInstance,f32 deltaTime);

void game_on_resize(Game* gameInstance,u32 width, u32 height);

