#pragma once
#include "vulkan_types.inl"

b8 vulkan_graphics_pipeline_create(VulkanContext* context, VulkanRenderpass* renderpass, u32 attribute_count, 
VkVertexInputAttributeDescription* attributes, u32 descriptorSetLayoutCount, VkDescriptorSetLayout* descriptorSetLayouts,
 u32 stageCount,VkPipelineShaderStageCreateInfo* stages,VkViewport viewport,VkRect2D scissor,b8 isWireframe,VulkanPipeline* outPipeline,int deviceIndex);

void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline, int deviceIndex);

void vulkan_pipeline_bind(VulkanCommandBuffer* commandBuffer, VkPipelineBindPoint bindPoint, VulkanPipeline* pipeline,int deviceIndex);