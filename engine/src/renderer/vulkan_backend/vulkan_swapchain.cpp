#include "renderer/vulkan_backend/vulkan_swapchain.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "renderer/vulkan_backend/vulkan_device.h"
#include "renderer/vulkan_backend/vulkan_image.h"
#include "renderer/vulkan_backend/vulkan_utils.h"

void create( VulkanContext *context, u32 width,u32 height,VulkanSwapchain *swapchain, int deviceIndex);
void destroy(VulkanContext *context, VulkanSwapchain *swapchain, int deviceIndex);

void vulkan_swapchain_create( VulkanContext *context, u32 width,u32 height,VulkanSwapchain *swapchain, int deviceIndex){
    create(context,width,height,swapchain,deviceIndex);
}

void vulkan_swapchain_recreate( VulkanContext *context, u32 width, u32 height, VulkanSwapchain *swapchain, int deviceIndex){
    destroy(context,swapchain,deviceIndex);
    create(context, width, height, swapchain,deviceIndex);
}

void vulkan_swapchain_destroy( VulkanContext *context, VulkanSwapchain *swapchain, int deviceIndex){
    destroy(context,swapchain,deviceIndex);
}

b8 vulkan_swapchain_acquire_next_image_index( VulkanContext *context, VulkanSwapchain *swapchain, u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32 *imageIndex, int deviceIndex){
  
    VkResult result = vkAcquireNextImageKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,timeoutNs,imageAvailableSemaphore,fence,&context->imageIndex[deviceIndex]);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        vulkan_swapchain_recreate(context,context->framebufferWidth[deviceIndex],context->framebufferHeight[deviceIndex],swapchain,deviceIndex);
        return FALSE;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        KFATAL("failed to aquire image");
        return FALSE;

    }
    // KINFO("Next Image result %s",vulkan_result_string(result,TRUE));
    return TRUE;

}

void vulkan_swapchain_present( VulkanContext *context, VulkanSwapchain *swapchain, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex, int deviceIndex){
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.pImageIndices = &presentImageIndex;
    presentInfo.pSwapchains = &swapchain->handle;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    
    VkResult result = vkQueuePresentKHR(presentQueue,&presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vulkan_swapchain_recreate(context,context->framebufferWidth[deviceIndex],context->framebufferHeight[deviceIndex],swapchain,deviceIndex);
    }
    else if(result != VK_SUCCESS){
        KFATAL("Failed to present SwapchainImage");
    }
     // Increment (and loop) the index.
    context->currentFrame[deviceIndex] = (context->currentFrame[deviceIndex] + 1) % swapchain->maxFramesInFlight;

}


void create( VulkanContext *context, u32 width,u32 height,VulkanSwapchain *swapchain, int deviceIndex){
    VkExtent2D swapchainExtent = {width, height};
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
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
    
    KDEBUG("Querying swapchain capabilities again");
    vulkan_device_query_swapchain_support(context->device.physicalDevices[deviceIndex],context->surface, &context->device.swapchainSupport);

     // Swapchain extent
    if (context->device.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        swapchainExtent = context->device.swapchainSupport.capabilities.currentExtent;
    }

    // Clamp to the value allowed by the GPU.
    VkExtent2D min = context->device.swapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchainSupport.capabilities.maxImageExtent;
    swapchainExtent.width = KCLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = KCLAMP(swapchainExtent.height, min.height, max.height);

    



    VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.minImageCount = context->device.swapchainSupport.capabilities.maxImageCount == 0 ? context->device.swapchainSupport.capabilities.minImageCount % 2 == 0 ? context->device.swapchainSupport.capabilities.minImageCount * 4 : (context->device.swapchainSupport.capabilities.minImageCount + 1) * 4 : context->device.swapchainSupport.capabilities.minImageCount ;
    createInfo.surface = context->surface;
    createInfo.imageFormat = swapchain->imageFormat.format;
    createInfo.imageColorSpace = swapchain->imageFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if(context->device.graphicsQueueIndex[deviceIndex] != context->device.presentQueueIndex[deviceIndex]){
        u32 queueFamilyIndices[] = {
            (u32)context->device.graphicsQueueIndex[deviceIndex],
            (u32)context->device.presentQueueIndex[deviceIndex]};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = 0;
    }
    createInfo.preTransform = context->device.swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logicalDevices[deviceIndex],&createInfo,context->allocator,&swapchain->handle));
    context->currentFrame[deviceIndex] = 0;
    swapchain->imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,&swapchain->imageCount,VK_NULL_HANDLE));
    swapchain->maxFramesInFlight = swapchain->imageCount - 1;
    swapchain->images = std::vector<VkImage>(swapchain->imageCount);
    swapchain->views = std::vector<VkImageView>(swapchain->imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logicalDevices[deviceIndex],swapchain->handle,&swapchain->imageCount,swapchain->images.data()));

     for (u32 i = 0; i < swapchain->imageCount; ++i) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchain->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain->imageFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logicalDevices[deviceIndex], &viewInfo, context->allocator, &swapchain->views[i]));

         
    }
    if (!vulkan_device_detect_depth_format(&context->device,deviceIndex)) {
        context->device.depthFormat = VK_FORMAT_UNDEFINED;
        KFATAL("Failed to find a supported format!");
    }


    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        swapchainExtent.width,
        swapchainExtent.height,
        context->device.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        TRUE,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depthAttachment,deviceIndex);

    KINFO("Swapchain created successfully for %s",context->device.properties[deviceIndex].deviceName);


    
}

void destroy(VulkanContext *context, VulkanSwapchain *swapchain, int deviceIndex){
    vkDeviceWaitIdle(context->device.logicalDevices[deviceIndex]);
     vulkan_image_destroy(context, &swapchain->depthAttachment,deviceIndex);

    // Only destroy the views, not the images, since those are owned by the swapchain and are thus
    // destroyed when it is.
    for (u32 i = 0; i < swapchain->imageCount; ++i) {
        vkDestroyImageView(context->device.logicalDevices[deviceIndex], swapchain->views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logicalDevices[deviceIndex], swapchain->handle, context->allocator);
}
