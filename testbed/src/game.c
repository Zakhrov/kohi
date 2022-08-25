#include "game.h"
#include <core/logger.h>
#include <memory/kmemory.h>
#include <core/input.h>


b8 game_initialize(Game* gameInstance){
    KDEBUG("Game Initialized called");
    return true;

}

b8 game_update(Game* gameInstance,f32 deltaTime){
    static u64 alloc_count = 0;
    u64 prev_alloc_count = alloc_count;
    alloc_count = get_memory_alloc_count();
    if (input_is_key_up('M') && input_was_key_down('M')) {
        KDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }
    // KDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    // KDEBUG("Game Update called");
    return true;
}

b8 game_render(Game* gameInstance,f32 deltaTime){
    // KDEBUG("Game Render called");
    return true;
}

void game_on_resize(Game* gameInstance,u32 width, u32 height){
    KDEBUG("Game On Resize called");
}

