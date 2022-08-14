#pragma once

#include "../defines.h"

typedef struct PlatformState
{
    void* internalState;
}PlatformState;

#ifdef __cplusplus

#include "../renderer/vulkan_backend/vulkan_platform.h"
extern "C"
{
#endif

b8 platform_startup(
    PlatformState* platformState,
    const char* applicationName,
    i32 x,
    i32 y,
    i32 width,
    i32 height);

void platform_shutdown(PlatformState* platformState);

b8 platform_pump_messages(PlatformState* platformState);

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

