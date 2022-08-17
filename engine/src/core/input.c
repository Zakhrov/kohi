#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct KeyboardState {
    b8 keys[256];
} KeyboardState;

typedef struct MouseState {
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} MouseState;

typedef struct InputState {
    KeyboardState keyboard_current;
    KeyboardState keyboard_previous;
    MouseState mouse_current;
    MouseState mouse_previous;
} InputState;


static b8 initialized = FALSE;
static InputState state = {};

void input_initialize() {
    kzero_memory(&state, sizeof(InputState));
    initialized = TRUE;
    KINFO("Input subsystem initialized.");
}

void input_shutdown() {
    // TODO: Add shutdown routines when needed.
    initialized = FALSE;
}

void input_update(f64 deltaTime) {
    if (!initialized) {
        return;
    }

    // Copy current states to previous states.
    kcopy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(KeyboardState));
    kcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(MouseState));
}


// keyboard input
b8 input_is_key_down(Keys key){
     if (!initialized) {
        return FALSE;
    }
    return state.keyboard_current.keys[key] == TRUE;
    

}
b8 input_is_key_up(Keys key){
     if (!initialized) {
        return TRUE;
    }
    return state.keyboard_current.keys[key] == FALSE;

}
b8 input_was_key_down(Keys key){
      if (!initialized) {
        return FALSE;
    }
    return state.keyboard_previous.keys[key] == TRUE;


}
b8 input_was_key_up(Keys key){
     if (!initialized) {
        return TRUE;
    }
    return state.keyboard_previous.keys[key] == FALSE;

}

void input_process_key(Keys key, b8 pressed){
     // Only handle this if the state actually changed.
    if (state.keyboard_current.keys[key] != pressed) {
        // Update internal state.
        state.keyboard_current.keys[key] = pressed;

        // Fire off an event for immediate processing.
        EventContext context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }

}

// mouse input
b8 input_is_button_down(MouseButtons button){

      if (!initialized) {
        return FALSE;
    }
    return state.mouse_current.buttons[button] == TRUE;

}
b8 input_is_button_up(MouseButtons button){
     if (!initialized) {
        return TRUE;
    }
    return state.mouse_current.buttons[button] == FALSE;

}
b8 input_was_button_down(MouseButtons button){
    if (!initialized) {
        return FALSE;
    }
    return state.mouse_previous.buttons[button] == TRUE;

}
b8 input_was_button_up(MouseButtons button){
    if (!initialized) {
        return TRUE;
    }
    return state.mouse_previous.buttons[button] == FALSE;

}
void input_get_mouse_position(i32* x, i32* y){
      if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;

}
void input_get_previous_mouse_position(i32* x, i32* y){
     if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;

}

void input_process_button(MouseButtons button, b8 pressed){
     // If the state changed, fire an event.
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;

        // Fire the event.
        EventContext context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }

}
void input_process_mouse_move(i16 x, i16 y){
     // Only process if actually different
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        // NOTE: Enable this if debugging.
        //KDEBUG("Mouse pos: %i, %i!", x, y);

        // Update internal state.
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        // Fire the event.
        EventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }

}
void input_process_mouse_wheel(i8 zDelta){

     // NOTE: no internal state to update.

    // Fire the event.
    EventContext context;
    context.data.u8[0] = zDelta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);

}