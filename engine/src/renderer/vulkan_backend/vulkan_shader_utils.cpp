#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "systems/resource_system.h"

b8 create_shader_module(VulkanContext* context, const char* name,const char* stageTypeString, VkShaderStageFlagBits shaderStageFlag, u32 stageIndex, VulkanShaderStage* shaderStages,int deviceIndex){

    // build filename

    char filename[512];
    string_format(filename,"shaders/%s.%s.spv",name,stageTypeString);
    kzero_memory(&shaderStages[stageIndex],sizeof(VkShaderModuleCreateInfo));
    shaderStages[stageIndex].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

     // Obtain file handle.
    Resource binaryResource;
    if(!resource_system_load(filename,RESOURCE_TYPE_BINARY,&binaryResource)){
        KERROR("unable to read shader module %s",filename);
    }
    kzero_memory(&shaderStages[stageIndex].shaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[stageIndex].createInfo.codeSize = binaryResource.dataSize;
    shaderStages[stageIndex].createInfo.pCode = (u32*)binaryResource.data;
    

    
    

    
    VK_CHECK(vkCreateShaderModule(
        context->device.logicalDevices[deviceIndex],
        &shaderStages[stageIndex].createInfo,
        context->allocator,
        &shaderStages[stageIndex].handle));

    resource_system_unload(&binaryResource);

    kzero_memory(&shaderStages[stageIndex].shaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[stageIndex].shaderStageCreateInfo.stage = shaderStageFlag;
    shaderStages[stageIndex].shaderStageCreateInfo.module = shaderStages[stageIndex].handle;
    shaderStages[stageIndex].shaderStageCreateInfo.pName = "main";
    
    return true;


}