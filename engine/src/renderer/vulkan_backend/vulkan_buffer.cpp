#include "renderer/vulkan_backend/vulkan_buffer.h"
#include "renderer/vulkan_backend/vulkan_command_buffer.h"
#include "core/logger.h"
#include "memory/kmemory.h"
b8 vulkan_buffer_create(
    VulkanContext* context,
    u64 size,
    VkBufferUsageFlags usage,
    VkMemoryAllocateFlags memoryPropertyFlags,
    b8 bindOnCreate,
    VulkanBuffer* buffer, int deviceIndex){

    kzero_memory(buffer, sizeof(VulkanBuffer));
    buffer->totalSize = size;
    buffer->usage = usage;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // NOTE: Only used in one queue.

    VK_CHECK(vkCreateBuffer(context->device.logicalDevices[deviceIndex], &buffer_info, context->allocator, &buffer->handle));

    // Gather memory requirements.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logicalDevices[deviceIndex], buffer->handle, &requirements);
    buffer->memoryIndex = context->findMemoryIndex(requirements.memoryTypeBits, buffer->memoryPropertyFlags,deviceIndex);
    if (buffer->memoryIndex == -1) {
        KERROR("Unable to create vulkan buffer because the required memory type index was not found.");
        return false;
    }

    // Allocate memory info
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)buffer->memoryIndex;

    // Allocate the memory.
    VkResult result = vkAllocateMemory(
        context->device.logicalDevices[deviceIndex],
        &allocate_info,
        context->allocator,
        &buffer->memory);

    if (result != VK_SUCCESS) {
        KERROR("Unable to create vulkan buffer because the required memory allocation failed. Error: %i", result);
        return false;
    }

    if (bindOnCreate) {
        vulkan_buffer_bind(context, buffer, 0,deviceIndex);
    }

    return true;

    }

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer,int deviceIndex){

        if (buffer->memory) {
        vkFreeMemory(context->device.logicalDevices[deviceIndex], buffer->memory, context->allocator);
        buffer->memory = 0;
    }
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logicalDevices[deviceIndex], buffer->handle, context->allocator);
        buffer->handle = 0;
    }
    buffer->totalSize = 0;
    
    buffer->isLocked = false;

}

b8 vulkan_buffer_resize(
    VulkanContext* context,
    u64 new_size,
    VulkanBuffer* buffer,
    VkQueue queue,
    VkCommandPool pool,int deviceIndex){
    
    // Create new buffer.
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = new_size;
    buffer_info.usage = buffer->usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // NOTE: Only used in one queue.


    VkBuffer new_buffer;
    VK_CHECK(vkCreateBuffer(context->device.logicalDevices[deviceIndex], &buffer_info, context->allocator, &new_buffer));

    // Gather memory requirements.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logicalDevices[deviceIndex], new_buffer, &requirements);

    // Allocate memory info
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)buffer->memoryIndex;

    // Allocate the memory.
    VkDeviceMemory new_memory;
    VkResult result = vkAllocateMemory(context->device.logicalDevices[deviceIndex], &allocate_info, context->allocator, &new_memory);
    if (result != VK_SUCCESS) {
        KERROR("Unable to resize vulkan buffer because the required memory allocation failed. Error: %i", result);
        return false;
    }

    // Bind the new buffer's memory
    VK_CHECK(vkBindBufferMemory(context->device.logicalDevices[deviceIndex], new_buffer, new_memory, 0));

    // Copy over the data
    vulkan_buffer_copy_to(context, pool, 0, queue, buffer->handle, 0, new_buffer, 0, buffer->totalSize,deviceIndex);

    // Make sure anything potentially using these is finished.
    vkDeviceWaitIdle(context->device.logicalDevices[deviceIndex]);

    // Destroy the old
    if (buffer->memory) {
        vkFreeMemory(context->device.logicalDevices[deviceIndex], buffer->memory, context->allocator);
        buffer->memory = 0;
    }
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logicalDevices[deviceIndex], buffer->handle, context->allocator);
        buffer->handle = 0;
    }

    // Set new properties
    buffer->totalSize = new_size;
    buffer->memory = new_memory;
    buffer->handle = new_buffer;

    return true;

    }

void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, u64 offset,int deviceIndex){

    VK_CHECK(vkBindBufferMemory(context->device.logicalDevices[deviceIndex], buffer->handle, buffer->memory, offset));
}

void* vulkan_buffer_lock_memory(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags,int deviceIndex){

    void* data;
    VK_CHECK(vkMapMemory(context->device.logicalDevices[deviceIndex], buffer->memory, offset, size, flags, &data));
    return data;

}
void vulkan_buffer_unlock_memory(VulkanContext* context, VulkanBuffer* buffer,int deviceIndex){

        vkUnmapMemory(context->device.logicalDevices[deviceIndex], buffer->memory);


}

void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags, const void* data,int deviceIndex){

    void* data_ptr;
    VK_CHECK(vkMapMemory(context->device.logicalDevices[deviceIndex], buffer->memory, offset, size, flags, &data_ptr));
    kcopy_memory(data_ptr, data, size);
    vkUnmapMemory(context->device.logicalDevices[deviceIndex], buffer->memory);

}

void vulkan_buffer_copy_to(
    VulkanContext* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size,int deviceIndex){

        // Create a one-time-use command buffer.
    VulkanCommandBuffer temp_command_buffer;
    vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &temp_command_buffer,deviceIndex,temp_command_buffer.id);

    // Prepare the copy command and add it to the command buffer.
    VkBufferCopy copy_region;
    copy_region.srcOffset = source_offset;
    copy_region.dstOffset = dest_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(temp_command_buffer.handle, source, dest, 1, &copy_region);

    // Submit the buffer for execution and wait for it to complete.
    vulkan_command_buffer_end_single_use(context, pool, &temp_command_buffer, queue,deviceIndex);

    }