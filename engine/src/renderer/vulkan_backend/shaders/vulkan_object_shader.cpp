#include "renderer/vulkan_backend/shaders/vulkan_object_shader.h"
#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "core/logger.h"

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
    return true;

}

void vulkan_object_shader_destroy(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex){
    for(int i=0; i < OBJECT_SHADER_STAGE_COUNT; i++){
        vkDestroyShaderModule(context->device.logicalDevices[deviceIndex],shader->stages[i].handle,context->allocator);
    }
    // vkDestroyPipeline(context->device.logicalDevices[deviceIndex],shader->pipeline.handle,context->allocator);

}

void vulkan_object_shader_use(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex){
    
}