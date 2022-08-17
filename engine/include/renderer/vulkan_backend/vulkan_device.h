#pragma once
#include "vulkan_types.inl"
b8 vulkan_device_create(VulkanContext* context);
void vulkan_device_destory(VulkanContext* context);

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VulkanSwapChainSupportInfo* swapchainSupportInfo); 

b8 vulkan_device_detect_depth_format(VulkanDevice* device,int deviceIndex);