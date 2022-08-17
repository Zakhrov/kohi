#include "renderer/vulkan_backend/vulkan_fence.h"
#include "core/logger.h"


void vulkan_fence_create(VulkanContext* context, b8 createSignaled, VulkanFence* fence,int deviceIndex){
    fence->isSignaled = createSignaled;
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if(fence->isSignaled){
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    vkCreateFence(context->device.logicalDevices[deviceIndex],&fenceCreateInfo,context->allocator,&fence->handle);

}

void vulkan_fence_destroy(VulkanContext* context, VulkanFence* fence,int deviceIndex){

    if(fence->handle!=VK_NULL_HANDLE){
        vkDestroyFence(context->device.logicalDevices[deviceIndex],fence->handle,context->allocator);
        KINFO("Destroy fence for %s",context->device.properties[deviceIndex].deviceName);
        fence->isSignaled = FALSE;
    }

}

b8 vulkan_fence_wait(VulkanContext* context, VulkanFence* fence, u64 timeoutNs,int deviceIndex){
    if(!fence->isSignaled){
        VkResult result = vkWaitForFences(context->device.logicalDevices[deviceIndex],1,&fence->handle,TRUE,timeoutNs);
        switch(result){
            case VK_SUCCESS:
                fence->isSignaled = TRUE;
                return TRUE;
             case VK_TIMEOUT:
                KWARN("vk_fence_wait - Timed out");
                break;
            case VK_ERROR_DEVICE_LOST:
                KERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST.");
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                KERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY.");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                KERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
                break;
            default:
                KERROR("vk_fence_wait - An unknown error has occurred.");
                break;
            
        }
        
    }
    else{
        return TRUE;
    }
    return FALSE;
}

void vulkan_fence_reset(VulkanContext* context, VulkanFence* fence, int deviceIndex){
    if(fence->isSignaled){
        VK_CHECK(vkResetFences(context->device.logicalDevices[deviceIndex],1,&fence->handle));
        fence->isSignaled = FALSE;
    }
}