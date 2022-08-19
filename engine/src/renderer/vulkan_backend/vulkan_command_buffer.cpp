#include "renderer/vulkan_backend/vulkan_command_buffer.h"

#include "core/kmemory.h"

void vulkan_command_buffer_allocate(VulkanContext* context, VkCommandPool pool,b8 isPrimary,VulkanCommandBuffer *commandBuffer,int deviceIndex,int bufferId){

    
    kzero_memory(commandBuffer,sizeof(VulkanCommandBuffer));
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = pool;
    allocateInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocateInfo.commandBufferCount = 1;

    commandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    commandBuffer->deviceIndex = deviceIndex;

    VK_CHECK(vkAllocateCommandBuffers(context->device.logicalDevices[deviceIndex],&allocateInfo,&commandBuffer->handle));
    commandBuffer->state = COMMAND_BUFFER_STATE_READY;

    

}

void vulkan_command_buffer_free(VulkanContext* context, VkCommandPool pool,VulkanCommandBuffer *commandBuffer,int deviceIndex){

    vkFreeCommandBuffers(
        context->device.logicalDevices[deviceIndex],
        pool,
        1,
        &commandBuffer->handle);

    commandBuffer->handle = 0;
    commandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;

}

void vulkan_command_buffer_begin(VulkanCommandBuffer* commandBuffer,b8 isSingleUse,b8 isRenderpassContinue,b8 isSimultaneousUse){

     VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = 0;
    if (isSingleUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (isRenderpassContinue) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (isSimultaneousUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_CHECK(vkBeginCommandBuffer(commandBuffer->handle, &beginInfo));
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;

}

void vulkan_command_buffer_end(VulkanCommandBuffer* commandBuffer){
    VK_CHECK(vkEndCommandBuffer(commandBuffer->handle));

}

void vulkan_command_buffer_update_submitted(VulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_SUBMITTED;

}

void vulkan_command_buffer_reset(VulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_READY;

}


void vulkan_command_buffer_allocate_and_begin_single_use(VulkanContext* context, VkCommandPool pool,VulkanCommandBuffer *commandBuffer,int deviceIndex,int bufferId){
    vulkan_command_buffer_allocate(context,pool,TRUE,commandBuffer,deviceIndex,bufferId);
    vulkan_command_buffer_begin(commandBuffer,TRUE,FALSE,FALSE);

}


void vulkan_command_buffer_end_single_use(VulkanContext* context,VkCommandPool pool,VulkanCommandBuffer* commandBuffer,VkQueue queue,int deviceIndex){

    //End the command buffer
    vulkan_command_buffer_end(commandBuffer);

    //Submit the Queue
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer->handle;
    VK_CHECK(vkQueueSubmit(queue,1,&submitInfo,VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(queue));

    // Free the command buffer.
    vulkan_command_buffer_free(context, pool, commandBuffer,deviceIndex);

}