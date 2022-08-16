#pragma once

#include "vulkan_types.inl"

void vulkan_fence_create(VulkanContext* context, b8 createSignaled, VulkanFence* fence,int deviceIndex);

void vulkan_fence_destroy(VulkanContext* context, VulkanFence* fence,int deviceIndex);

b8 vulkan_fence_wait(VulkanContext* context, VulkanFence* fence, u64 timeoutNs,int deviceIndex);

void vulkan_fence_reset(VulkanContext* context, VulkanFence* fence, int deviceIndex);