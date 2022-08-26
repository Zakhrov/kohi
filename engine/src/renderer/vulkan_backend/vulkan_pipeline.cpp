#include "renderer/vulkan_backend/vulkan_pipeline.h"
#include "memory/kmemory.h"
#include "math/math_types.h"
#include "core/logger.h"
#include "renderer/vulkan_backend/vulkan_utils.h"
b8 vulkan_graphics_pipeline_create(VulkanContext* context, VulkanRenderpass* renderpass, u32 attribute_count, 
VkVertexInputAttributeDescription* attributes, u32 descriptorSetLayoutCount, VkDescriptorSetLayout* descriptorSetLayouts,
 u32 stageCount,VkPipelineShaderStageCreateInfo* stages,VkViewport viewport,VkRect2D scissor,b8 isWireframe,VulkanPipeline* outPipeline, int deviceIndex){

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Multisampling.
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateInfo.minSampleShading = 1.0f;
    multisamplingCreateInfo.pSampleMask = 0;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    // Depth and stencil testing.
    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
    kzero_memory(&colorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    

    // Dynamic state
    const u32 dynamicStateCount = 3;
    VkDynamicState dynamic_states[dynamicStateCount] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
    dynamicStateCreateInfo.pDynamicStates = dynamic_states;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;  // Binding index
    bindingDescription.stride = sizeof(Vertex3D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Move to next data entry for each vertex.

    // Attributes
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attribute_count;
    vertexInputInfo.pVertexAttributeDescriptions = attributes;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    VkPipelineLayoutCreateInfo layoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutCreateInfo.pSetLayouts = descriptorSetLayouts;
    layoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
    VK_CHECK(vkCreatePipelineLayout(context->device.logicalDevices[deviceIndex],&layoutCreateInfo,context->allocator,&outPipeline->pipelineLayout));

    // Pipeline create
    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    graphicsPipelineCreateInfo.stageCount = stageCount;
    graphicsPipelineCreateInfo.pStages = stages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;

    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = 0;

    graphicsPipelineCreateInfo.layout = outPipeline->pipelineLayout;

    graphicsPipelineCreateInfo.renderPass = renderpass->handle;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(context->device.logicalDevices[deviceIndex],VK_NULL_HANDLE,1,&graphicsPipelineCreateInfo,context->allocator,&outPipeline->handle);

     if (vulkan_result_is_success(result)) {
        KDEBUG("Graphics pipeline created!");
        return true;
    }

    KERROR("vkCreateGraphicsPipelines failed with %s.", vulkan_result_string(result, true));
    return false;


 }

void vulkan_pipeline_destroy(VulkanContext* context, VulkanPipeline* pipeline,int deviceIndex){
    vkDestroyPipelineLayout(context->device.logicalDevices[deviceIndex],pipeline->pipelineLayout,context->allocator);
    vkDestroyPipeline(context->device.logicalDevices[deviceIndex],pipeline->handle,context->allocator);

}

void vulkan_pipeline_bind(VulkanCommandBuffer* commandBuffer, VkPipelineBindPoint bindPoint, VulkanPipeline* pipeline,int deviceIndex){
    vkCmdBindPipeline(commandBuffer->handle,bindPoint,pipeline->handle);

}