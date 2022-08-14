#pragma once

#include "../../defines.h"
#include "../../kohi_asserts.h"

#include <vulkan/vulkan.h>
#include <vector>


typedef struct VulkanSwapChainSupportInfo{
   VkSurfaceCapabilitiesKHR capabilities;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 presentModeCount;
    VkPresentModeKHR* presentModes;
}VulkanSwapChainSupportInfo;


typedef struct VulkanDevice{
    std::vector<VkPhysicalDevice> physicalDevices;
    std::vector<VkDevice> logicalDevices;
    std::vector<const char*> deviceNames;
    u32 deviceCount;
    VulkanSwapChainSupportInfo swapchainSupport;
    std::vector<u32> graphicsQueueIndex;
    std::vector<u32> presentQueueIndex;
    std::vector<u32> transferQueueIndex;
    std::vector<VkQueue> graphicsQueues;
    std::vector<VkQueue> transferQueues;
    std::vector<VkQueue> presentQueues;
    std::vector<VkPhysicalDeviceProperties> properties;
    std::vector<VkPhysicalDeviceFeatures> features;
    std::vector<VkPhysicalDeviceMemoryProperties> memory;
}VulkanDevice;


typedef struct VulkanContext {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VulkanDevice device;
    VkSurfaceKHR surface;
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
} VulkanContext;

#define VK_CHECK(expr){ KASSERT(expr == VK_SUCCESS);}