#pragma once
#include "vulkan_types.inl"



b8 vulkan_buffer_create(
    VulkanContext* context,
    u64 size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryPropertyFlags,
    b8 bindOnCreate,
    VulkanBuffer* buffer, int deviceIndex);

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer,int deviceIndex);

b8 vulkan_buffer_resize(
    VulkanContext* context,
    u64 new_size,
    VulkanBuffer* buffer,
    VkQueue queue,
    VkCommandPool pool,int deviceIndex);

void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, u64 offset,int deviceIndex);

void* vulkan_buffer_lock_memory(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags,int deviceIndex);
void vulkan_buffer_unlock_memory(VulkanContext* context, VulkanBuffer* buffer,int deviceIndex);

void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags, const void* data,int deviceIndex);

void vulkan_buffer_copy_to(
    VulkanContext* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size,int deviceIndex);