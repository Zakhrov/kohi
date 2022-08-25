#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "platform/filesystem.h"
b8 create_shader_module(VulkanContext* context, const char* name,const char* stageTypeString, VkShaderStageFlagBits shaderStageFlag, u32 stageIndex, VulkanShaderStage* shaderStages,int deviceIndex){

    // build filename

    char filename[512];
    string_format(filename,"../assets/shaders/%s.%s.glsl.spv",name,stageTypeString);
    kzero_memory(&shaderStages[stageIndex],sizeof(VkShaderModuleCreateInfo));
    shaderStages[stageIndex].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

     // Obtain file handle.
    FileHandle handle;
    if (!filesystem_open(filename, FILE_MODE_READ, true, &handle)) {
        KERROR("Unable to read shader module: %s.", filename);
        return false;
    }

    // Read the entire file as binary.
    u64 size = 0;
    u8* fileBuffer = 0;
    if (!filesystem_read_all_bytes(&handle, &fileBuffer, &size)) {
        KERROR("Unable to binary read shader module: %s.", filename);
        return false;
    }
    shaderStages[stageIndex].createInfo.codeSize = size;
    shaderStages[stageIndex].createInfo.pCode = (u32*)fileBuffer;

    // Close the file.
    filesystem_close(&handle);

    VK_CHECK(vkCreateShaderModule(
        context->device.logicalDevices[deviceIndex],
        &shaderStages[stageIndex].createInfo,
        context->allocator,
        &shaderStages[stageIndex].handle));

    // Shader stage info
    kzero_memory(&shaderStages[stageIndex].shaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[stageIndex].shaderStageCreateInfo.stage = shaderStageFlag;
    shaderStages[stageIndex].shaderStageCreateInfo.module = shaderStages[stageIndex].handle;
    shaderStages[stageIndex].shaderStageCreateInfo.pName = "main";

    if (fileBuffer) {
        kfree(fileBuffer, sizeof(u8) * size, MEMORY_TAG_STRING);
        fileBuffer = 0;
    }

    return true;


}