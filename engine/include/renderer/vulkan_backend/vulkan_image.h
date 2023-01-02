#pragma once
#include "vulkan_types.inl"
#include "memory/kmemory.h"
void vulkan_image_create(
    VulkanContext* context,
    VkImageType imageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    b32 createView,
    VkImageAspectFlags viewAspectFlags,
    VulkanImage* outImage,int deviceIndex);


void vulkan_image_view_create(
    VulkanContext* context,
    VkFormat format,
    VulkanImage* image,
    VkImageAspectFlags aspectFlags,int deviceIndex);

/**
 * Transitions the provided image from old_layout to new_layout.
 */
void vulkan_image_transition_layout(
    VulkanContext* context,
    VulkanCommandBuffer* commandBuffer,
    VulkanImage* image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout, int deviceIndex);

/**
 * Copies data in buffer to provided image.
 * @param context The Vulkan context.
 * @param image The image to copy the buffer's data to.
 * @param buffer The buffer whose data will be copied.
 */
void vulkan_image_copy_from_buffer(
    VulkanContext* context,
    VulkanImage* image,
    VkBuffer buffer,
    VulkanCommandBuffer* commandBuffer,int deviceIndex);


void vulkan_image_destroy(VulkanContext* context, VulkanImage* image,int deviceIndex);