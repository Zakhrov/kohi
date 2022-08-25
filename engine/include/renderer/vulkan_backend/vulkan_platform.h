#pragma once
#include "../../defines.h"
#include "../../platform/platform_linux.inl"
#ifdef __cplusplus
#include "vulkan_types.inl"
extern "C"
{
#endif
void platform_get_required_extension_names(std::vector<const char*>* extensions); 
b8 platform_create_vulkan_surface(PlatformState* platformState,VulkanContext* context);
#ifdef __cplusplus
}
#endif
