#pragma once

#include "../defines.h"


#ifdef __cplusplus
extern "C"
{
#endif



void platform_system_startup(
    u64* memoryRequirement,
    void* platformState,
    const char* applicationName,
    i32 x,
    i32 y,
    i32 width,
    i32 height);

void platform_system_shutdown(void* platformState);

b8 platform_pump_messages(void* platformState);

void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

void platform_console_write(const char* message, u8 colour);
void platform_console_write_error(const char* message, u8 colour);

f64 platform_get_absolute_time();

// Sleep on the thread for the provided ms. This blocks the main thread.
// Should only be used for giving time back to the OS for unused update power.
// Therefore it is not exported.
void platform_sleep(u64 ms); 



#ifdef __cplusplus
}
#endif

