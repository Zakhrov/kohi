#include "renderer/vulkan_backend/shaders/vulkan_object_shader.h"
#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "renderer/vulkan_backend/vulkan_pipeline.h"
#include "core/logger.h"
#include "memory/kmemory.h"
#include "renderer/vulkan_backend/vulkan_buffer.h"
#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

b8 vulkan_object_shader_create(VulkanContext *context, VulkanObjectShader *shader, int deviceIndex)
{
    // Shader module init per state
    char stageTypeStrings[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stageTypes[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
    {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stageTypeStrings[i], stageTypes[i], i, shader->stages, deviceIndex))
        {
            KERROR("Unable to create %s shader module for '%s'.", stageTypeStrings[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }

    // Global Descriptors
    VkDescriptorSetLayoutBinding globalDescriptorSetLayoutBinding;
    globalDescriptorSetLayoutBinding.binding = 0;
    globalDescriptorSetLayoutBinding.descriptorCount = 1;
    globalDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalDescriptorSetLayoutBinding.pImmutableSamplers = 0;
    globalDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo globalDescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    globalDescriptorSetLayoutCreateInfo.bindingCount = 1;
    globalDescriptorSetLayoutCreateInfo.pBindings = &globalDescriptorSetLayoutBinding;
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logicalDevices[deviceIndex], &globalDescriptorSetLayoutCreateInfo, context->allocator, &shader->descriptorSetLayout));

    VkDescriptorPoolSize globalDescriptorPoolSize;
    globalDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalDescriptorPoolSize.descriptorCount = context->swapchains[deviceIndex].imageCount;

    VkDescriptorPoolCreateInfo globalDescriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    globalDescriptorPoolCreateInfo.poolSizeCount = 1;
    globalDescriptorPoolCreateInfo.pPoolSizes = &globalDescriptorPoolSize;
    globalDescriptorPoolCreateInfo.maxSets = context->swapchains[deviceIndex].imageCount;

    VK_CHECK(vkCreateDescriptorPool(context->device.logicalDevices[deviceIndex], &globalDescriptorPoolCreateInfo, context->allocator, &shader->descriptorPool));

    // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebufferHeight[deviceIndex];
    viewport.width = (f32)context->framebufferWidth[deviceIndex];
    viewport.height = -(f32)context->framebufferHeight[deviceIndex];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebufferWidth[deviceIndex];
    scissor.extent.height = context->framebufferHeight[deviceIndex];

    // Attributes
    u32 offset = 0;
    const i32 attributeCount = 1;
    VkVertexInputAttributeDescription attributeDescriptions[attributeCount];
    // Position
    VkFormat formats[attributeCount] = {
        VK_FORMAT_R32G32B32_SFLOAT};
    u64 sizes[attributeCount] = {
        sizeof(glm::vec3)};
    for (u32 i = 0; i < attributeCount; ++i)
    {
        attributeDescriptions[i].binding = 0;  // binding index - should match binding desc
        attributeDescriptions[i].location = i; // attrib location
        attributeDescriptions[i].format = formats[i];
        attributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Desciptor set layouts.
    const i32 descriptorSetLayoutCount = 1;
    VkDescriptorSetLayout layouts[1] = {shader->descriptorSetLayout};

    // Stages
    // NOTE: Should match the number of shader->stages.
    VkPipelineShaderStageCreateInfo stageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
    kzero_memory(stageCreateInfos, sizeof(stageCreateInfos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i)
    {
        stageCreateInfos[i].sType = shader->stages[i].shaderStageCreateInfo.sType;
        stageCreateInfos[i] = shader->stages[i].shaderStageCreateInfo;
    }

    if (!vulkan_graphics_pipeline_create(context, &context->mainRenderPasses[deviceIndex], attributeCount, attributeDescriptions, descriptorSetLayoutCount, layouts, OBJECT_SHADER_STAGE_COUNT, stageCreateInfos, viewport, scissor, false, &shader->pipeline, deviceIndex))
    {
        KERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }
    // Create uniform buffer
    if (!vulkan_buffer_create(context, sizeof(GlobalUniformObject) * 3,
                              (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              true,
                              &shader->globalUniformBuffer,
                              deviceIndex))
    {
        KERROR("Vulkan buffer creation failed for object shader");
        return false;
    }

    // Allocate global descriptor sets
    VkDescriptorSetLayout globalLayouts[3] = {shader->descriptorSetLayout,
                                        shader->descriptorSetLayout,
                                        shader->descriptorSetLayout};

    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = shader->descriptorPool;
    allocInfo.descriptorSetCount = 3;
    allocInfo.pSetLayouts = globalLayouts;

    VK_CHECK(vkAllocateDescriptorSets(context->device.logicalDevices[deviceIndex],&allocInfo,shader->descriptorSets));


    return true;
}

void vulkan_object_shader_destroy(VulkanContext *context, VulkanObjectShader *shader, int deviceIndex)
{
    // Destroy global uniform buffer
    vulkan_buffer_destroy(context,&shader->globalUniformBuffer,deviceIndex);

    // Destroy shader pipeline
    vulkan_pipeline_destroy(context, &shader->pipeline, deviceIndex);

    // Destroy Descriptor pool
    vkDestroyDescriptorPool(context->device.logicalDevices[deviceIndex],shader->descriptorPool,context->allocator);
    // Destroy Descriptor set layouts
    vkDestroyDescriptorSetLayout(context->device.logicalDevices[deviceIndex],shader->descriptorSetLayout,context->allocator);
    
    for (int i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
    {
        vkDestroyShaderModule(context->device.logicalDevices[deviceIndex], shader->stages[i].handle, context->allocator);
    }
    
}

void vulkan_object_shader_use(VulkanContext *context, VulkanObjectShader *shader, int deviceIndex)
{

    u32 imageIndex = context->imageIndex[deviceIndex];
    // Bind shader pipeline to command buffers
    vulkan_pipeline_bind(&context->graphicsCommandBuffers[deviceIndex][imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline, deviceIndex);
}

void vulkan_object_shader_update_global_state(VulkanContext* context,VulkanObjectShader* shader, int deviceIndex){
    u32 imageIndex = context->imageIndex[deviceIndex];
    VkCommandBuffer commandBuffer = context->graphicsCommandBuffers[deviceIndex][imageIndex].handle;
    VkDescriptorSet descriptorSet = shader->descriptorSets[imageIndex];
    
    // Configure descriptors for global index
    u32 range = sizeof(GlobalUniformObject);
    u64 offset = sizeof(GlobalUniformObject) * imageIndex;

    // Copy data to buffer
    vulkan_buffer_load_data(context,&shader->globalUniformBuffer,offset,range,0,&shader->globalUBO,deviceIndex);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = shader->globalUniformBuffer.handle;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Update descriptor sets
    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.dstSet = shader->descriptorSets[imageIndex];
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context->device.logicalDevices[deviceIndex],1,&writeDescriptorSet,0,0);

    // Bind global descriptor sets
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,shader->pipeline.pipelineLayout,0,1,&descriptorSet,0,0);
    
    



}

void vulkan_object_shader_update_object(VulkanContext* context, VulkanObjectShader* shader,mat4 model,int deviceIndex){
    u32 imageIndex = context->imageIndex[deviceIndex];
    VkCommandBuffer commandBuffer = context->graphicsCommandBuffers[deviceIndex][imageIndex].handle;
    vkCmdPushConstants(commandBuffer,shader->pipeline.pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(mat4),&model);

}