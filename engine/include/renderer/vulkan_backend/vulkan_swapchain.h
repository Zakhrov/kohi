#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(VulkanContext* context,u32 width, u32 height, VulkanSwapchain* swapchain,int deviceIndex);
void vulkan_swapchain_recreate(VulkanContext* context,u32 width, u32 height, VulkanSwapchain* swapchain,int deviceIndex);

void vulkan_swapchain_destroy(VulkanContext* context,VulkanSwapchain* swapchain,int deviceIndex);

b8 vulkan_swapchain_acquire_next_image_index(VulkanContext* context, VulkanSwapchain* swapchain, u64 timeoutMs, VkSemaphore imageAvailableSemaphore,VkFence fence,u32* imageIndex,int deviceIndex);

void vulkan_swapchain_present(VulkanContext* context, VulkanSwapchain* swapchain,VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderSemaphore,u32 presentImageIndex,int deviceIndex);
