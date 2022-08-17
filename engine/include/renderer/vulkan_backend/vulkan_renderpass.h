#pragma once

#include "vulkan_types.inl"

void vulkan_renderpass_create(VulkanContext* context, VulkanRenderpass* renderpass, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a, f32 depth, u32 stencil,int deviceIndex);

void vulkan_renderpass_destroy(VulkanContext* context, VulkanRenderpass* renderpass,int deviceIndex);

void vulkan_renderpass_begin(VulkanCommandBuffer* commandBuffer, VulkanRenderpass* renderpass,VkFramebuffer frameBuffer );

void vulkan_renderpass_end(VulkanCommandBuffer *commandBuffer, VulkanRenderpass* renderpass);