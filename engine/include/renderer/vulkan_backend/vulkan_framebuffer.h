#pragma once

#include "vulkan_types.inl"

void vulkan_framebuffer_create(VulkanContext* context, VulkanRenderpass* renderpass,u32 width,u32 height,u32 attachmentCount,std::vector<VkImageView> attachments,VulkanFramebuffer* framebuffer,int deviceIndex);

void vulkan_framebuffer_destroy(VulkanContext* context, VulkanFramebuffer* framebuffer,int deviceIndex);


