#pragma once
#include "../../defines.h"
struct PlatformState;
struct VulkanContext;
#ifdef __cplusplus
#include <vector>
extern "C"
{
#endif
void platform_get_required_extension_names(std::vector<const char*>* extensions); 
b8 platform_create_vulkan_surface(struct PlatformState* platformState,struct VulkanContext* context);
#ifdef __cplusplus
}
#endif
