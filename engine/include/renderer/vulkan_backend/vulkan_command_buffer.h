#pragma once

#include "vulkan_types.inl"

void vulkan_command_buffer_allocate(VulkanContext* context, VkCommandPool pool,b8 isPrimary,VulkanCommandBuffer *commandBuffer,int deviceIndex);

void vulkan_command_buffer_free(VulkanContext* context, VkCommandPool pool,VulkanCommandBuffer *commandBuffer,int deviceIndex);

void vulkan_command_buffer_begin(VulkanCommandBuffer* commandBuffer,b8 isSingleUse,b8 isRenderpassContinue,b8 isSimultaneousUse);

void vulkan_command_buffer_end(VulkanCommandBuffer* commandBuffer);

void vulkan_command_buffer_update_submitted(VulkanCommandBuffer* commandBuffer);

void vulkan_command_buffer_reset(VulkanCommandBuffer* commandBuffer);

/**
 * Allocates and begins recording to out_command_buffer.
 */
void vulkan_command_buffer_allocate_and_begin_single_use(VulkanContext* context, VkCommandPool pool,VulkanCommandBuffer *commandBuffer,int deviceIndex);

/**
 * Ends recording, submits to and waits for queue operation and frees the provided command buffer.
 */
void vulkan_command_buffer_end_single_use(
    VulkanContext* context,
    VkCommandPool pool,
    VulkanCommandBuffer* commandBuffer,
    VkQueue queue, int deviceIndex); 