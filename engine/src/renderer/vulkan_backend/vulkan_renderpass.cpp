#include "renderer/vulkan_backend/vulkan_renderpass.h"
#include "memory/kmemory.h"
#include "core/logger.h"
void vulkan_renderpass_create(VulkanContext* context, VulkanRenderpass* renderpass, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a, f32 depth, u32 stencil,int deviceIndex){

    renderpass->a = a;
    renderpass->b = b;
    renderpass->depth = depth;
    renderpass->g = g;
    renderpass->h = h;
    renderpass->r = r;
    renderpass->stencil = stencil;
    renderpass->w = w;
    renderpass->x = x;
    renderpass->y = y;
    renderpass->deviceIndex = deviceIndex;

    // Main subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;


    // Attachments TODO: Make this configurable
    u32 attachmentDescriptionCount =2;
    VkAttachmentDescription attacnmentDescriptions[attachmentDescriptionCount];

     // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = context->swapchains[deviceIndex].imageFormat.format; // TODO: configurable
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Do not expect any particular layout before render pass starts.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Transitioned to after the render pass
    colorAttachment.flags = 0;

    attacnmentDescriptions[0] = colorAttachment;

    VkAttachmentReference colorAttachmentReference;
    colorAttachmentReference.attachment = 0;  // Attachment description array index
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    // Depth attachment if one is available
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = context->device.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attacnmentDescriptions[1] = depthAttachment;

     // Depth attachment reference
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // TODO: other attachment types (input, resolve, preserve)

    // Depth stencil data.
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    // Input from a shader
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;

    // Attachments used for multisampling colour attachments
    subpass.pResolveAttachments = 0;

    // Attachments not used in this subpass, but must be preserved for the next.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;

    // Render pass dependencies. TODO: make this configurable.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderpassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderpassCreateInfo.attachmentCount = attachmentDescriptionCount;
    renderpassCreateInfo.pAttachments = attacnmentDescriptions;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = &subpass;
    renderpassCreateInfo.dependencyCount = 1;
    renderpassCreateInfo.pDependencies = &dependency;
    
    
    
    VK_CHECK(vkCreateRenderPass(context->device.logicalDevices[deviceIndex],&renderpassCreateInfo,context->allocator,&renderpass->handle));
    KINFO("Created Renderpass for %s",context->device.properties[deviceIndex].deviceName);
}

void vulkan_renderpass_destroy(VulkanContext* context, VulkanRenderpass* renderpass,int deviceIndex){
    vkDestroyRenderPass(context->device.logicalDevices[deviceIndex],renderpass->handle,context->allocator);
    renderpass->handle = 0;
    KINFO("Destroyed Renderpass for %s",context->device.properties[deviceIndex].deviceName);
}

void vulkan_renderpass_begin(VulkanCommandBuffer* commandBuffer, VulkanRenderpass* renderpass,VkFramebuffer frameBuffer ){

    // KDEBUG("Recording renderpass on commandbuffer index %i on device index %i",commandBuffer->id,commandBuffer->deviceIndex);

    VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    beginInfo.renderPass = renderpass->handle;
    beginInfo.framebuffer = frameBuffer;
    beginInfo.renderArea.offset.x = renderpass->x;
    beginInfo.renderArea.offset.y = renderpass->y;
    beginInfo.renderArea.extent.width = renderpass->w;
    beginInfo.renderArea.extent.height = renderpass->h;


    VkClearValue clearValues[2];
    kzero_memory(clearValues,sizeof(VkClearValue) *2);
    clearValues[0].color.float32[0] = renderpass->r;
    clearValues[0].color.float32[1] = renderpass->g;
    clearValues[0].color.float32[2] = renderpass->b;
    clearValues[0].color.float32[3] = renderpass->a;
    clearValues[1].depthStencil.depth = renderpass->depth;
    clearValues[1].depthStencil.stencil = renderpass->stencil;

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(commandBuffer->handle,&beginInfo,VK_SUBPASS_CONTENTS_INLINE);
}

void vulkan_renderpass_end(VulkanCommandBuffer *commandBuffer, VulkanRenderpass* renderpass){
    vkCmdEndRenderPass(commandBuffer->handle);
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}