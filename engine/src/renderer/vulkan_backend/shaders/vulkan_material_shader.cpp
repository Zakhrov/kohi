#include "renderer/vulkan_backend/shaders/vulkan_material_shader.h"
#include "renderer/vulkan_backend/vulkan_shader_utils.h"
#include "renderer/vulkan_backend/vulkan_pipeline.h"
#include "core/logger.h"
#include "memory/kmemory.h"
#include "renderer/vulkan_backend/vulkan_buffer.h"
#include "math/kmath.h"
#include "systems/texture_system.h"

#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"

b8 vulkan_material_shader_create(VulkanContext *context, VulkanMaterialShader *shader, int deviceIndex)
{
    
    // Shader module init per state
    char stageTypeStrings[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stageTypes[MATERIAL_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; i++)
    {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_MATERIAL, stageTypeStrings[i], stageTypes[i], i, shader->stages, deviceIndex))
        {
            KERROR("Unable to create %s shader module for '%s'.", stageTypeStrings[i], BUILTIN_SHADER_NAME_MATERIAL);
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

    // Sampler uses.
    shader->samplerUses[0] = TEXTURE_USE_MAP_DIFFUSE;
    // Local/Object Descriptors
    
    VkDescriptorType descriptorTypes[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // Binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // Binding 1 - Diffuse sampler layout.
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
    kzero_memory(&bindings, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT);
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = descriptorTypes[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT;
    layoutInfo.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logicalDevices[deviceIndex], &layoutInfo, 0, &shader->objectDescriptorSetLayout));

    // Local/Object descriptor pool: Used for object-specific items like diffuse colour
    VkDescriptorPoolSize objectPoolSizes[2];
    // The first section will be used for uniform buffers
    objectPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectPoolSizes[0].descriptorCount = VULKAN_MAX_MATERIAL_COUNT;
    // The second section will be used for image samplers.
    objectPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    objectPoolSizes[1].descriptorCount = VULKAN_MATERIAL_SHADER_SAMPLER_COUNT * VULKAN_MAX_MATERIAL_COUNT;

    VkDescriptorPoolCreateInfo objectPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    objectPoolInfo.poolSizeCount = 2;
    objectPoolInfo.pPoolSizes = objectPoolSizes;
    objectPoolInfo.maxSets = VULKAN_MAX_MATERIAL_COUNT;
    objectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Create object descriptor pool.
    VK_CHECK(vkCreateDescriptorPool(context->device.logicalDevices[deviceIndex], &objectPoolInfo, context->allocator, &shader->objectDescriptorPool));

    
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
    #define ATTRIBUTE_COUNT 2
    VkVertexInputAttributeDescription attributeDescriptions[ATTRIBUTE_COUNT];
    // Position, texcoord
    VkFormat formats[ATTRIBUTE_COUNT] = {
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT};
    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(vec3),
        sizeof(vec2)};
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        attributeDescriptions[i].binding = 0;  // binding index - should match binding desc
        attributeDescriptions[i].location = i; // attrib location
        attributeDescriptions[i].format = formats[i];
        attributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Desciptor set layouts.
    const i32 descriptorSetLayoutCount = 2;
    VkDescriptorSetLayout layouts[2] = {shader->descriptorSetLayout,shader->objectDescriptorSetLayout};

    // Stages
    // NOTE: Should match the number of shader->stages.
    VkPipelineShaderStageCreateInfo stageCreateInfos[MATERIAL_SHADER_STAGE_COUNT];
    kzero_memory(stageCreateInfos, sizeof(stageCreateInfos));
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i)
    {
        stageCreateInfos[i].sType = shader->stages[i].shaderStageCreateInfo.sType;
        stageCreateInfos[i] = shader->stages[i].shaderStageCreateInfo;
    }

    if (!vulkan_graphics_pipeline_create(context, &context->mainRenderPasses[deviceIndex], ATTRIBUTE_COUNT, attributeDescriptions, descriptorSetLayoutCount, layouts, MATERIAL_SHADER_STAGE_COUNT, stageCreateInfos, viewport, scissor, false, &shader->pipeline, deviceIndex))
    {
        KERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }
    // Create uniform buffer
    
    if (!vulkan_buffer_create(context, sizeof(GlobalUniformObject) * 3,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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

    // Create the object uniform buffer.
    if (!vulkan_buffer_create(
            context,
            sizeof(MaterialUniformObject) * VULKAN_MAX_MATERIAL_COUNT,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &shader->objectUniformBuffer,deviceIndex)) {
        KERROR("Material instance buffer creation failed for shader.");
        return false;
    }


    return true;
}

void vulkan_material_shader_destroy(VulkanContext *context, VulkanMaterialShader *shader, int deviceIndex)
{

    // Destroy Descriptor pool
    vkDestroyDescriptorPool(context->device.logicalDevices[deviceIndex],shader->descriptorPool,context->allocator);
    // Destroy Descriptor set layouts
    vkDestroyDescriptorSetLayout(context->device.logicalDevices[deviceIndex],shader->descriptorSetLayout,context->allocator);

    // Destroy Object Descriptor pool
    vkDestroyDescriptorPool(context->device.logicalDevices[deviceIndex],shader->objectDescriptorPool,context->allocator);
    // Destroy Object Descriptor set layouts
    vkDestroyDescriptorSetLayout(context->device.logicalDevices[deviceIndex],shader->objectDescriptorSetLayout,context->allocator);

    // Destroy global uniform buffer
    vulkan_buffer_destroy(context,&shader->globalUniformBuffer,deviceIndex);
    // destroy object uniform buffer
    vulkan_buffer_destroy(context,&shader->objectUniformBuffer,deviceIndex);

    // Destroy shader pipeline
    vulkan_pipeline_destroy(context, &shader->pipeline, deviceIndex);

    
    
    for (int i = 0; i < MATERIAL_SHADER_STAGE_COUNT; i++)
    {
        vkDestroyShaderModule(context->device.logicalDevices[deviceIndex], shader->stages[i].handle, context->allocator);
    }
    
}

void vulkan_material_shader_use(VulkanContext *context, VulkanMaterialShader *shader, int deviceIndex)
{

    u32 imageIndex = context->imageIndex[deviceIndex];
    // Bind shader pipeline to command buffers
    vulkan_pipeline_bind(&context->graphicsCommandBuffers[deviceIndex][imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline, deviceIndex);
}

void vulkan_material_shader_update_global_state(VulkanContext* context,VulkanMaterialShader* shader, f64 deltaTime, int deviceIndex){
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
void vulkan_material_shader_set_model(VulkanContext* context, VulkanMaterialShader* shader, mat4 model, int deviceIndex){
    if (context && shader) {
        u32 image_index = context->imageIndex[deviceIndex];
        VkCommandBuffer command_buffer = context->graphicsCommandBuffers[deviceIndex][image_index].handle;

        vkCmdPushConstants(command_buffer, shader->pipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
    }
}

void vulkan_material_shader_apply_material(VulkanContext* context, VulkanMaterialShader* shader,Material* material,int deviceIndex){
    if(context && shader){

    
    
    u32 imageIndex = context->imageIndex[deviceIndex];
    VkCommandBuffer commandBuffer = context->graphicsCommandBuffers[deviceIndex][imageIndex].handle;
    // Obtain material data
    VulkanMaterialShaderInstanceState* instanceState = &shader->instanceStates[material->internalId];
    VkDescriptorSet object_descriptor_set = instanceState->descriptorSets[imageIndex];

    // TODO: if needs update
    VkWriteDescriptorSet descriptorWrites[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
    kzero_memory(descriptorWrites, sizeof(VkWriteDescriptorSet) * VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT);
    u32 descriptorCount = 0;
    u32 descriptorIndex = 0;

    // Descriptor 0 - Uniform buffer
    u32 range = sizeof(MaterialUniformObject);
    u64 offset = sizeof(MaterialUniformObject) * material->internalId;  // also the index into the array.
    MaterialUniformObject obo;

    // TODO: get diffuse colour from a material.
    // static f32 accumulator = 0.0f;
    // accumulator += context->frameDeltaTime;
    // // f32 s = (ksin(accumulator) + 1.0f) / 2.0f;  // scale from -1, 1 to 0, 1
    // f32 s = 1.0f;
    // obo.diffuseColor = vec4_create(s, s, s, 1.0f);
    obo.diffuseColor = material->diffuseColour;

    // Load the data into the buffer.
    
    vulkan_buffer_load_data(context, &shader->objectUniformBuffer, offset, range, 0, &obo,deviceIndex);
    

    // Only do this if the descriptor has not yet been updated.
    u32* globalUboGeneration = &instanceState->descriptorStates[descriptorIndex].generations[imageIndex];
    if (*globalUboGeneration == INVALID_ID || *globalUboGeneration != material->generation ) {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->objectUniformBuffer.handle;
        buffer_info.offset = offset;
        buffer_info.range = range;

        VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor.dstSet = object_descriptor_set;
        descriptor.dstBinding = descriptorIndex;
        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor.descriptorCount = 1;
        descriptor.pBufferInfo = &buffer_info;

        descriptorWrites[descriptorCount] = descriptor;
        descriptorCount++;

        // Update the frame generation. In this case it is only needed once since this is a buffer.
        *globalUboGeneration = material->generation;
    }
    descriptorIndex++;

    // Samplers.
    VkDescriptorImageInfo image_infos[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];
    for (u32 sampler_index = 0; sampler_index < VULKAN_MATERIAL_SHADER_SAMPLER_COUNT; ++sampler_index) {
        TextureUse use = shader->samplerUses[sampler_index];
        Texture* t = 0;
        switch (use) {
            case TEXTURE_USE_MAP_DIFFUSE:
                t = material->diffuseMap.texture;
                break;
            default:
                KFATAL("Unable to bind sampler to unknown use.");
                return;
        }
        u32* descriptor_generation = &instanceState->descriptorStates[descriptorIndex].generations[imageIndex];
        u32* descriptor_id = &instanceState->descriptorStates[descriptorIndex].ids[imageIndex];

                // If the texture hasn't been loaded yet, use the default.

        // TODO: Determine which use the texture has and pull appropriate default based on that.
        if (t->generation == INVALID_ID) {
            t = texture_system_get_default_texture();

            // Reset the descriptor generation if using the default texture.
            *descriptor_generation = INVALID_ID;
        }

        // Check if the descriptor needs updating first.
        if (t && ( *descriptor_id != t->id || *descriptor_generation != t->generation || *descriptor_generation == INVALID_ID)) {
            VulkanTexture* internalData = (VulkanTexture*)t->internalData;

            // Assign view and sampler.
            image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[sampler_index].imageView = internalData->textureData[deviceIndex]->image.view;
            image_infos[sampler_index].sampler = internalData->textureData[deviceIndex]->sampler;

            VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = object_descriptor_set;
            descriptor.dstBinding = descriptorIndex;
            descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &image_infos[sampler_index];

            descriptorWrites[descriptorCount] = descriptor;
            descriptorCount++;

            // Sync frame generation if not using a default texture.
            if (t->generation != INVALID_ID) {
                *descriptor_generation = t->generation;
                *descriptor_id = t->id;
            }
            descriptorIndex++;
        }
    }

    if (descriptorCount > 0) {
        vkUpdateDescriptorSets(context->device.logicalDevices[deviceIndex], descriptorCount, descriptorWrites, 0, 0);

    }

    // Bind the descriptor set to be updated, or in case the shader changed.

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.pipelineLayout, 1, 1, &object_descriptor_set, 0, 0);

    }
}

b8 vulkan_material_shader_acquire_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material, int deviceIndex){
     // TODO: free list
    material->internalId = shader->objectUniformBufferIndex;
    shader->objectUniformBufferIndex++;
    KDEBUG("Material internal Id %d",material->internalId);

    // u32 objectId = *outObjectId;
    VulkanMaterialShaderInstanceState* instanceState = &shader->instanceStates[material->internalId];
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            instanceState->descriptorStates[i].generations[j] = INVALID_ID;
            instanceState->descriptorStates[i].ids[j] = INVALID_ID;
        }
    }

        // Allocate descriptor sets.
    VkDescriptorSetLayout layouts[3] = {
        shader->objectDescriptorSetLayout,
        shader->objectDescriptorSetLayout,
        shader->objectDescriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = shader->objectDescriptorPool;
    allocInfo.descriptorSetCount = 3;  // one per frame
    allocInfo.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(context->device.logicalDevices[deviceIndex], &allocInfo, instanceState->descriptorSets);
    if (result != VK_SUCCESS) {
        KERROR("Error allocating descriptor sets in shader!");
        return false;
    }


    return true;
}

void vulkan_material_shader_release_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material, int deviceIndex){
    vkDeviceWaitIdle(context->device.logicalDevices[deviceIndex]);
    KDEBUG("Material internal Id %d",material->internalId);
    VulkanMaterialShaderInstanceState* instanceState = &shader->instanceStates[material->internalId];

    const u32 descriptorSetCount = 3;
    // Release object descriptor sets.
    VkResult result = vkFreeDescriptorSets(context->device.logicalDevices[deviceIndex], shader->objectDescriptorPool, descriptorSetCount, instanceState->descriptorSets);
    if (result != VK_SUCCESS) {
        KERROR("Error freeing object shader descriptor sets!");
    }

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            instanceState->descriptorStates[i].generations[j] = INVALID_ID;
            instanceState->descriptorStates[i].ids[j] = INVALID_ID;

        }
    }

    // TODO: add the object_id to the free list
}