#pragma once
#include "vulkan_types.inl"

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

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image,int deviceIndex);