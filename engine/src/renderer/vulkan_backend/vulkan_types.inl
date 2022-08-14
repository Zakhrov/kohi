#pragma once

#include "../../defines.h"
#include "../../kohi_asserts.h"

#include <vulkan/vulkan.h>
#include <vector>

typedef struct VulkanDevice{
    std::vector<VkPhysicalDevice> physicalDevices;
    std::vector<VkDevice> logicalDevices;
}VulkanDevice;

typedef struct VulkanContext {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
} VulkanContext;

#define VK_CHECK(expr){ KASSERT(expr == VK_SUCCESS);}