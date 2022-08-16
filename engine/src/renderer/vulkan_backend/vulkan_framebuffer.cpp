#include "vulkan_framebuffer.h"

#include "../../core/kmemory.h"

void vulkan_framebuffer_create(VulkanContext* context, VulkanRenderpass* renderpass,u32 width,u32 height,u32 attachmentCount,VkImageView* attachments,VulkanFramebuffer* framebuffer,int deviceIndex){
    framebuffer->attachments = (VkImageView*)kallocate(sizeof(VkImageView)*attachmentCount,MEMORY_TAG_RENDERER);
    for(int i=0; i < attachmentCount; i++){
        framebuffer->attachments[i] = attachments[i];
    }
    framebuffer->renderpass = renderpass;
    framebuffer->attachmentCount = attachmentCount;
    VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferCreateInfo.attachmentCount = attachmentCount;
    framebufferCreateInfo.renderPass = renderpass->handle;
    framebufferCreateInfo.pAttachments = framebuffer->attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(context->device.logicalDevices[deviceIndex],&framebufferCreateInfo,context->allocator,&framebuffer->handle));
}

void vulkan_framebuffer_destroy(VulkanContext* context, VulkanFramebuffer* framebuffer,int deviceIndex){
    vkDestroyFramebuffer(context->device.logicalDevices[deviceIndex],framebuffer->handle,context->allocator);
}