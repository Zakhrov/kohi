#include "renderer/vulkan_backend/vulkan_image.h"
#include "renderer/vulkan_backend/vulkan_device.h"
#include "core/logger.h"
void vulkan_image_create(
    VulkanContext *context,
    VkImageType imageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    b32 createView,
    VkImageAspectFlags viewAspectFlags,
    VulkanImage *outImage, int deviceIndex)
{

    outImage->width = width;
    outImage->height = height;
    VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1; // TODO: support configurable depth
    imageCreateInfo.mipLevels = 4;    // TODO: support mip mapping
    imageCreateInfo.arrayLayers = 1;  // TODO: support configurable array layers
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // TODO: configurable sample count
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // TODO: configurable sharing mode

    VK_CHECK(vkCreateImage(context->device.logicalDevices[deviceIndex], &imageCreateInfo, context->allocator, &outImage->handle));
    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(context->device.logicalDevices[deviceIndex], outImage->handle, &memoryRequirements);
    i32 memoryType = context->findMemoryIndex(memoryRequirements.memoryTypeBits, memoryFlags,deviceIndex);
    if (memoryType == -1)
    {
        KERROR("Required memory type not found image is not valid");
    }

    VkMemoryAllocateInfo memoryAllocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = memoryType;
    // Allocate Memory
    VK_CHECK(vkAllocateMemory(context->device.logicalDevices[deviceIndex], &memoryAllocateInfo, context->allocator, &outImage->memory));

    VK_CHECK(vkBindImageMemory(context->device.logicalDevices[deviceIndex], outImage->handle, outImage->memory, 0)); // TODO: configurable memory offset

    if (createView)
    {
        outImage->view = VK_NULL_HANDLE;
        vulkan_image_view_create(context, format, outImage, viewAspectFlags, deviceIndex);
    }
}

void vulkan_image_view_create(
    VulkanContext *context,
    VkFormat format,
    VulkanImage *image,
    VkImageAspectFlags aspectFlags, int deviceIndex)
{

    VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.image = image->handle;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // TODO: Make configurable.
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    // TODO: Make configurable
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device.logicalDevices[deviceIndex], &viewCreateInfo, context->allocator, &image->view));
}

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image,int deviceIndex){
    if(image->view){
        vkDestroyImageView(context->device.logicalDevices[deviceIndex],image->view,context->allocator);
        image->view = 0;
    }
    if(image->memory){
        vkFreeMemory(context->device.logicalDevices[deviceIndex],image->memory,context->allocator);
        image->memory = 0;
    }
    if(image->handle){
        vkDestroyImage(context->device.logicalDevices[deviceIndex],image->handle,context->allocator);
        image->handle = 0;
    }
}