#pragma once

#include "vulkan_types.inl"

b8 create_shader_module(VulkanContext* context, const char* name,const char* stageTypeString, VkShaderStageFlagBits shaderStageFlag, u32 stageIndex, VulkanShaderStage* shaderStages,int deviceIndex);