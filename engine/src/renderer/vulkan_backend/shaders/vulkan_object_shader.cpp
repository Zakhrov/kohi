#include "renderer/vulkan_backend/shaders/vulkan_object_shader.h"
#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "renderer/vulkan_backend/vulkan_pipeline.h"
#include "core/logger.h"
#include "memory/kmemory.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"


b8 vulkan_object_shader_create(VulkanContext* context, VulkanObjectShader* shader, int deviceIndex){
    // Shader module init per state
    char stageTypeStrings[OBJECT_SHADER_STAGE_COUNT][5] = {"vert","frag"};
    VkShaderStageFlagBits stageTypes[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_FRAGMENT_BIT}; 
    for(u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++){
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stageTypeStrings[i], stageTypes[i], i, shader->stages,deviceIndex)) {
            KERROR("Unable to create %s shader module for '%s'.", stageTypeStrings[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }
    //TODO Descriptors

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
        VK_FORMAT_R32G32B32_SFLOAT
    };
    u64 sizes[attributeCount] = {
        sizeof(glm::vec3)
    };
    for (u32 i = 0; i < attributeCount; ++i) {
        attributeDescriptions[i].binding = 0;   // binding index - should match binding desc
        attributeDescriptions[i].location = i;  // attrib location
        attributeDescriptions[i].format = formats[i];
        attributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // TODO: Desciptor set layouts.

    // Stages
    // NOTE: Should match the number of shader->stages.
    VkPipelineShaderStageCreateInfo stageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
    kzero_memory(stageCreateInfos, sizeof(stageCreateInfos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        stageCreateInfos[i].sType = shader->stages[i].shaderStageCreateInfo.sType;
        stageCreateInfos[i] = shader->stages[i].shaderStageCreateInfo;
    }

    if(!vulkan_graphics_pipeline_create(context,&context->mainRenderPasses[deviceIndex],attributeCount,attributeDescriptions,0,0,OBJECT_SHADER_STAGE_COUNT,stageCreateInfos,viewport,scissor,false,&shader->pipeline,deviceIndex)){
         KERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }

    return true;

}

void vulkan_object_shader_destroy(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex){
    vulkan_pipeline_destroy(context,&shader->pipeline,deviceIndex);
    for(int i=0; i < OBJECT_SHADER_STAGE_COUNT; i++){
        vkDestroyShaderModule(context->device.logicalDevices[deviceIndex],shader->stages[i].handle,context->allocator);
    }
    

}

void vulkan_object_shader_use(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex){
    
}