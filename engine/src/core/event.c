#include "core/event.h"
#include "memory/kmemory.h"
#include "containers/darray.h"
typedef struct RegisteredEvent{
    void* listener;
    PFN_on_event callback;
}RegisteredEvent;

typedef struct EventCodeEntry{
    RegisteredEvent* events;

}EventCodeEntry;

#define MAX_MESSAGE_CODES 16384

typedef struct EventSystemState{
    EventCodeEntry registered[MAX_MESSAGE_CODES];
}EventSystemState;


static EventSystemState* eventSystemStatePtr;

void event_system_initialize(u64* memoryRequirement, void* state){
    *memoryRequirement =sizeof(EventSystemState);
    if(state == 0){
        return;
    }

    kzero_memory(state,sizeof(state));
    eventSystemStatePtr = state;
    
}
void event_system_shutdown(void* state){

    if(eventSystemStatePtr){
        // Free the events arrays. And objects pointed to should be destroyed on their own.
        for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
            if(eventSystemStatePtr->registered[i].events != 0) {
                darray_destroy(eventSystemStatePtr->registered[i].events);
                eventSystemStatePtr->registered[i].events = 0;
            }
        }
    }

    


}
b8 event_register(u16 code, void* listener, PFN_on_event on_event){

     if(!eventSystemStatePtr) {
        return false;
    }

    if(eventSystemStatePtr->registered[code].events == 0) {
        eventSystemStatePtr->registered[code].events = darray_create(RegisteredEvent);
    }

    u64 registered_count = darray_length(eventSystemStatePtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        if(eventSystemStatePtr->registered[code].events[i].listener == listener) {
            // TODO: warn
            return false;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    RegisteredEvent event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(eventSystemStatePtr->registered[code].events, event);

    return true;

}
b8 event_unregister(u16 code, void* listener, PFN_on_event on_event){
     if(!eventSystemStatePtr) {
        return false;
    }

    // On nothing is registered for the code, boot out.
    if(eventSystemStatePtr->registered[code].events == 0) {
        // TODO: warn
        return false;
    }

    u64 registered_count = darray_length(eventSystemStatePtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        RegisteredEvent e = eventSystemStatePtr->registered[code].events[i];
        if(e.listener == listener && e.callback == on_event) {
            // Found one, remove it
            RegisteredEvent popped_event;
            darray_pop_at(eventSystemStatePtr->registered[code].events, i, &popped_event);
            return true;
        }
    }

    // Not found.
    return false;

}
b8 event_fire(u16 code, void* sender, EventContext context){
     if(!eventSystemStatePtr) {
        return false;
    }

    // If nothing is registered for the code, boot out.
    if(eventSystemStatePtr->registered[code].events == 0) {
        return false;
    }

    u64 registered_count = darray_length(eventSystemStatePtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        RegisteredEvent e = eventSystemStatePtr->registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Message has been handled, do not send to other listeners.
            return true;
        }
    }

    // Not found.
    return false;

}
