#include "core/input.h"
#include "core/event.h"
#include "memory/kmemory.h"
#include "core/logger.h"

typedef struct KeyboardState
{
    b8 keys[256];
} KeyboardState;

typedef struct MouseState
{
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} MouseState;

typedef struct InputState
{
    KeyboardState keyboard_current;
    KeyboardState keyboard_previous;
    MouseState mouse_current;
    MouseState mouse_previous;
} InputState;

static InputState* statePtr = 0;

void input_system_initialize(u64* memoryRequirement, void* state)
{   
    *memoryRequirement = sizeof(InputState);
    if(state == 0){
        return;
    }
    kzero_memory(state, sizeof(InputState));
    statePtr = state;
    
    KINFO("Input subsystem initialized.");
}

void input_system_shutdown(void* state)
{
    // TODO: Add shutdown routines when needed.
   statePtr = 0;
}

void input_update(f64 deltaTime)
{
    if (!statePtr)
    {
        return;
    }

    // Copy current states to previous states.
    kcopy_memory(&statePtr->keyboard_previous, &statePtr->keyboard_current, sizeof(KeyboardState));
    kcopy_memory(&statePtr->mouse_previous, &statePtr->mouse_current, sizeof(MouseState));
}

// keyboard input
b8 input_is_key_down(Keys key)
{
    if (!statePtr)
    {
        return false;
    }
    return statePtr->keyboard_current.keys[key] == true;
}
b8 input_is_key_up(Keys key)
{
    if (!statePtr)
    {
        return true;
    }
    return statePtr->keyboard_current.keys[key] == false;
}
b8 input_was_key_down(Keys key)
{
    if (!statePtr)
    {
        return false;
    }
    return statePtr->keyboard_previous.keys[key] == true;
}
b8 input_was_key_up(Keys key)
{
    if (!statePtr)
    {
        return true;
    }
    return statePtr->keyboard_previous.keys[key] == false;
}

void input_process_key(Keys key, b8 pressed)
{
    // Only handle this if the state actually changed.
    if (statePtr->keyboard_current.keys[key] != pressed)
    {
        // Update internal statePtr->
        statePtr->keyboard_current.keys[key] = pressed;

        if (key == KEY_LALT)
        {
            KINFO("Left alt pressed.");
        }
        else if (key == KEY_RALT)
        {
            KINFO("Right alt pressed.");
        }

        if (key == KEY_LCONTROL)
        {
            KINFO("Left ctrl pressed.");
        }
        else if (key == KEY_RCONTROL)
        {
            KINFO("Right ctrl pressed.");
        }

        if (key == KEY_LSHIFT)
        {
            KINFO("Left shift pressed.");
        }
        else if (key == KEY_RSHIFT)
        {
            KINFO("Right shift pressed.");
        }

        // Fire off an event for immediate processing.
        EventContext context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

// mouse input
b8 input_is_button_down(MouseButtons button)
{

    if (!statePtr)
    {
        return false;
    }
    return statePtr->mouse_current.buttons[button] == true;
}
b8 input_is_button_up(MouseButtons button)
{
    if (!statePtr)
    {
        return true;
    }
    return statePtr->mouse_current.buttons[button] == false;
}
b8 input_was_button_down(MouseButtons button)
{
    if (!statePtr)
    {
        return false;
    }
    return statePtr->mouse_previous.buttons[button] == true;
}
b8 input_was_button_up(MouseButtons button)
{
    if (!statePtr)
    {
        return true;
    }
    return statePtr->mouse_previous.buttons[button] == false;
}
void input_get_mouse_position(i32 *x, i32 *y)
{
    if (!statePtr)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = statePtr->mouse_current.x;
    *y = statePtr->mouse_current.y;
}
void input_get_previous_mouse_position(i32 *x, i32 *y)
{
    if (!statePtr)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = statePtr->mouse_previous.x;
    *y = statePtr->mouse_previous.y;
}

void input_process_button(MouseButtons button, b8 pressed)
{
    // If the state changed, fire an event.
    if (statePtr->mouse_current.buttons[button] != pressed)
    {
        statePtr->mouse_current.buttons[button] = pressed;

        // Fire the event.
        EventContext context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}
void input_process_mouse_move(i16 x, i16 y)
{
    // Only process if actually different
    if (statePtr->mouse_current.x != x || statePtr->mouse_current.y != y)
    {
        // NOTE: Enable this if debugging.
        // KDEBUG("Mouse pos: %i, %i!", x, y);

        // Update internal statePtr->
        statePtr->mouse_current.x = x;
        statePtr->mouse_current.y = y;

        // Fire the event.
        EventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}
void input_process_mouse_wheel(i8 zDelta)
{

    // NOTE: no internal state to update.

    // Fire the event.
    EventContext context;
    context.data.u8[0] = zDelta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}