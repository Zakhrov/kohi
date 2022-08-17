#include "renderer/vulkan_backend/vulkan_swapchain.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "renderer/vulkan_backend/vulkan_device.h"
#include "renderer/vulkan_backend/vulkan_image.h"

void create(VulkanContext* context,u32 width,u32 height,VulkanSwapchain *swapchain,int deviceIndex);
void destroy(VulkanContext* context, VulkanSwapchain* swapchain,int deviceIndex);

void vulkan_swapchain_create(VulkanContext* context,u32 width, u32 height, VulkanSwapchain* swapchain,int deviceIndex){
    create(context,width,height,swapchain,deviceIndex);
}
void vulkan_swapchain_recreate(VulkanContext* context,u32 width, u32 height, VulkanSwapchain* swapchain,int deviceIndex){
    destroy(context,swapchain,deviceIndex);
    create(context,width,height,swapchain,deviceIndex);

}
void vulkan_swapchain_destroy(VulkanContext* context,VulkanSwapchain* swapchain,int deviceIndex){
    destroy(context,swapchain,deviceIndex);
}

b8 vulkan_swapchain_acquire_next_image_index(VulkanContext* context, VulkanSwapchain* swapchain, u64 timeoutMs, VkSemaphore imageAvailableSemaphore,VkFence fence,u32* imageIndex,int deviceIndex){
    VkResult result = vkAcquireNextImageKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,timeoutMs,imageAvailableSemaphore,fence,imageIndex);
    context->swapchainResult = result;
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Trigger swapchain recreation, then boot out of the render loop.
        vulkan_swapchain_recreate(context, context->framebufferWidth[deviceIndex], context->framebufferHeight[deviceIndex], swapchain,deviceIndex);
        return FALSE;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        KFATAL("Failed to acquire swapchain image!");
        return FALSE;
    }

    return TRUE;
}

void vulkan_swapchain_present(VulkanContext* context, VulkanSwapchain* swapchain,VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderSemaphore,u32 presentImageIndex,int deviceIndex){
    // Return the image to the swapchain for presentation.
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &renderSemaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &presentImageIndex;
    present_info.pResults = 0;

    VkResult result = vkQueuePresentKHR(presentQueue, &present_info);
    // context->swapchainResult = result;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
        vulkan_swapchain_recreate(context, context->framebufferWidth[deviceIndex], context->framebufferHeight[deviceIndex], swapchain,deviceIndex);
    } else if (result != VK_SUCCESS) {
        KFATAL("Failed to present swap chain image!");
    }
     // Increment (and loop) the index.
    context->currentFrame[deviceIndex] = (context->currentFrame[deviceIndex] + 1) % swapchain->maxFramesInFlight;
}

void create(VulkanContext* context,u32 width,u32 height,VulkanSwapchain* swapchain,int deviceIndex){
    VkExtent2D swapchainExtent = {width,height};
    swapchain->maxFramesInFlight = 2;
     b8 found = FALSE;
    for (u32 i = 0; i < context->device.swapchainSupport.formatCount; ++i) {
        VkSurfaceFormatKHR format = context->device.swapchainSupport.formats[i];
        // Preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->imageFormat = format;
            found = TRUE;
            break;
        }
    }
     if (!found) {
        swapchain->imageFormat = context->device.swapchainSupport.formats[0];
    }

      VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    // for (u32 i = 0; i < context->device.swapchainSupport.presentModeCount; ++i) {
    //     VkPresentModeKHR mode = context->device.swapchainSupport.presentModes[i];
    //     if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //         presentMode = mode;
    //         break;
    //     }
    // }

    // Requery swapchain support.
    vulkan_device_query_swapchain_support(
        context->device.physicalDevices[deviceIndex],
        context->surface,
        &context->device.swapchainSupport);

    // Swapchain extent
    if (context->device.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        swapchainExtent = context->device.swapchainSupport.capabilities.currentExtent;
    }

     // Clamp to the value allowed by the GPU.
    VkExtent2D min = context->device.swapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchainSupport.capabilities.maxImageExtent;
    swapchainExtent.width = KCLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = KCLAMP(swapchainExtent.height, min.height, max.height);

    u32 imageCount = context->device.swapchainSupport.capabilities.minImageCount + 1;
     if (context->device.swapchainSupport.capabilities.maxImageCount > 0 && imageCount > context->device.swapchainSupport.capabilities.maxImageCount) {
        imageCount = context->device.swapchainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = context->surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = swapchain->imageFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchain->imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup Queue family indices
    if(context->device.graphicsQueueIndex[deviceIndex] != context->device.presentQueueIndex[deviceIndex]){
        u32 queueFamilyIndices[] = {
            (u32)context->device.graphicsQueueIndex[deviceIndex],
            (u32)context->device.presentQueueIndex[deviceIndex]};
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = 0;
    }
    swapchainCreateInfo.preTransform = context->device.swapchainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logicalDevices[deviceIndex],&swapchainCreateInfo,context->allocator,&swapchain->handle));
    context->currentFrame[deviceIndex] = 0;
    context->imageIndex[deviceIndex] = 0;
    swapchain->imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,&swapchain->imageCount,VK_NULL_HANDLE));
    if(!swapchain->images){
        swapchain->images = (VkImage*)kallocate(sizeof(VkImage)*swapchain->imageCount,MEMORY_TAG_RENDERER);
    }
    if(!swapchain->views){
        swapchain->views = (VkImageView*)kallocate(sizeof(VkImageView)*swapchain->imageCount,MEMORY_TAG_RENDERER); 
    }
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,&swapchain->imageCount,swapchain->images));

    for(int i=0; i < swapchain->imageCount; i++){
        VkImageViewCreateInfo viewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewCreateInfo.image = swapchain->images[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = swapchain->imageFormat.format;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logicalDevices[deviceIndex], &viewCreateInfo, context->allocator, &swapchain->views[i]));

    }
     // Depth resources
    if (!vulkan_device_detect_depth_format(&context->device,deviceIndex)) {
        context->device.depthFormat = VK_FORMAT_UNDEFINED;
        KFATAL("Failed to find a supported format!");
    }
    // Create Depth image
    vulkan_image_create(context,VK_IMAGE_TYPE_2D,swapchainExtent.width,swapchainExtent.height,context->device.depthFormat,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,TRUE,VK_IMAGE_ASPECT_DEPTH_BIT,&swapchain->depthAttachment,deviceIndex);
    KINFO("Swapchain created On %s",context->device.properties[deviceIndex].deviceName);


}

void destroy(VulkanContext* context, VulkanSwapchain* swapchain,int deviceIndex){
    vkDeviceWaitIdle(context->device.logicalDevices[deviceIndex]);
    vulkan_image_destroy(context,&swapchain->depthAttachment,deviceIndex);

    // Only destroy the views and not the images
    for(int i=0; i< swapchain->imageCount; i++){
        vkDestroyImageView(context->device.logicalDevices[deviceIndex],swapchain->views[i],context->allocator);
    }
    vkDestroySwapchainKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,context->allocator);


}

