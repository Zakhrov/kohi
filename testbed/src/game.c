#include "game.h"
#include <core/logger.h>
#include <memory/kmemory.h>
#include <core/input.h>

// HACK: Should be imported outside the engine!!!!
#include <renderer/renderer_frontend.h>
#include <math/kmath.h>

void recalculate_camera_view(GameState* state){
    if(state->cameraViewDirty){
      mat4 rotation = mat4_euler_xyz(state->cameraEuler.x,state->cameraEuler.y,state->cameraEuler.z);
      mat4 translation = mat4_translation(state->cameraPosition);
      state->view = mat4_inverse(mat4_mul(rotation,translation));
      state->cameraViewDirty = false;
    }


    
}

void camera_pitch(GameState* state,f32 amount){
    state->cameraEuler.x += amount;
    // Clamp value +/- 89 deg to prevent Gimball lock
    f32 limit = deg_to_rad(89);
    state->cameraEuler.x = KCLAMP(state->cameraEuler.x,-limit,limit);
    state->cameraViewDirty = true;

}

void camera_yaw(GameState* state,f32 amount){
    state->cameraEuler.y += amount;
    // Clamp value +/- 89 deg to prevent Gimball lock
    f32 limit = deg_to_rad(89);
    state->cameraEuler.y = KCLAMP(state->cameraEuler.y,-limit,limit);

    state->cameraViewDirty = true;

}
void camera_roll(GameState* state,f32 amount){
    state->cameraEuler.z += amount;
    // // Clamp value +/- 89 deg to prevent Gimball lock
    // f32 limit = deg_to_rad(89);
    // state->cameraEuler.z = KCLAMP(state->cameraEuler.z,-limit,limit);
    state->cameraViewDirty = true;

}

b8 game_initialize(Game* gameInstance){
    GameState* state = (GameState*)gameInstance->state;
    state->cameraPosition = (vec3){0,0,30.0f};
    state->cameraEuler = vec3_zero();
    state->cameraViewDirty = true;


    KDEBUG("Game Initialized called");
    return true;

}

b8 game_update(Game* gameInstance,f32 deltaTime){
    static u64 alloc_count = 0;
    GameState* state = (GameState*)gameInstance->state;
    u64 prev_alloc_count = alloc_count;
    alloc_count = get_memory_alloc_count();
    if (input_is_key_up('M') && input_was_key_down('M')) {
        KDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }
    // HACK: Temp code to move the camera around
    if(input_is_key_down(KEY_A) || input_is_key_down(KEY_LEFT)){
        camera_yaw(state,1.0f * deltaTime);
    }
    if(input_is_key_down(KEY_D) || input_is_key_down(KEY_RIGHT)){
        camera_yaw(state, -1.0f * deltaTime);
    }
    if(input_is_key_down(KEY_UP)){
        camera_pitch(state,1.0f * deltaTime);
    }
    if(input_is_key_down(KEY_DOWN)){
        camera_pitch(state, -1.0f * deltaTime);
    }
    
    f32 tempMoveSpeed = 10.0f;
    vec3 velocity = vec3_zero();
    if(input_is_key_down(KEY_W)){
        vec3 forwardVector = mat4_forward(state->view);
        velocity = vec3_add(velocity,forwardVector);
        
    }
    if(input_is_key_down(KEY_S)){
        vec3 backwardVector = mat4_backward(state->view);
        velocity = vec3_add(velocity,backwardVector);
    }
    if(input_is_key_down(KEY_Q)){
        vec3 leftVector = mat4_left(state->view);
        velocity = vec3_add(velocity,leftVector);
    }
    if(input_is_key_down(KEY_E) ){
        vec3 rightVector = mat4_right(state->view);
        velocity = vec3_add(velocity,rightVector);
    }
    if(input_is_key_down(KEY_SPACE)){
        velocity.y += 1.0f;
    }
    if(input_is_key_down(KEY_X)){
        velocity.y -= 1.0f;
    }
    vec3 z = vec3_zero();
    if(!vec3_compare(z,velocity,0.0002f)){
        // Always normalize velocity before applying it
        vec3_normalize(&velocity);
        state->cameraPosition.x += velocity.x * tempMoveSpeed * deltaTime;
        state->cameraPosition.y += velocity.y * tempMoveSpeed * deltaTime;
        state->cameraPosition.z += velocity.z * tempMoveSpeed * deltaTime;
        state->cameraViewDirty = true;

    }
    recalculate_camera_view(state);
    // HACK: THIS SHOULD NOT BE AVAILABLE OUTSIDE THE ENGINE!!!!
    renderer_set_view(state->view);
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

