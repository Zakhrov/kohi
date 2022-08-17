#include "core/event.h"
#include "core/kmemory.h"
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

static b8 isInitialized = FALSE;
static EventSystemState eventSystemState;

b8 event_initialize(){
    if(isInitialized == TRUE){
        return FALSE;
    }
    isInitialized = FALSE;
    kzero_memory(&eventSystemState,sizeof(eventSystemState));
    isInitialized = TRUE;

    return TRUE;
}
void event_shutdown(){

    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(eventSystemState.registered[i].events != 0) {
            darray_destroy(eventSystemState.registered[i].events);
            eventSystemState.registered[i].events = 0;
        }
    }


}
b8 event_register(u16 code, void* listener, PFN_on_event on_event){

     if(isInitialized == FALSE) {
        return FALSE;
    }

    if(eventSystemState.registered[code].events == 0) {
        eventSystemState.registered[code].events = darray_create(RegisteredEvent);
    }

    u64 registered_count = darray_length(eventSystemState.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        if(eventSystemState.registered[code].events[i].listener == listener) {
            // TODO: warn
            return FALSE;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    RegisteredEvent event;
    event.listener = listener;
    event.callback = on_event;
    darray_push(eventSystemState.registered[code].events, event);

    return TRUE;

}
b8 event_unregister(u16 code, void* listener, PFN_on_event on_event){
     if(isInitialized == FALSE) {
        return FALSE;
    }

    // On nothing is registered for the code, boot out.
    if(eventSystemState.registered[code].events == 0) {
        // TODO: warn
        return FALSE;
    }

    u64 registered_count = darray_length(eventSystemState.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        RegisteredEvent e = eventSystemState.registered[code].events[i];
        if(e.listener == listener && e.callback == on_event) {
            // Found one, remove it
            RegisteredEvent popped_event;
            darray_pop_at(eventSystemState.registered[code].events, i, &popped_event);
            return TRUE;
        }
    }

    // Not found.
    return FALSE;

}
b8 event_fire(u16 code, void* sender, EventContext context){
     if(isInitialized == FALSE) {
        return FALSE;
    }

    // If nothing is registered for the code, boot out.
    if(eventSystemState.registered[code].events == 0) {
        return FALSE;
    }

    u64 registered_count = darray_length(eventSystemState.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        RegisteredEvent e = eventSystemState.registered[code].events[i];
        if(e.callback(code, sender, e.listener, context)) {
            // Message has been handled, do not send to other listeners.
            return TRUE;
        }
    }

    // Not found.
    return FALSE;

}
