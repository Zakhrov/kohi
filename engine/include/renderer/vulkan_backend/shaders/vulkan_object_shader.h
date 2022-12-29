#pragma once
#include "../vulkan_types.inl"
#include "../../renderer_types.inl"

b8 vulkan_object_shader_create(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);

void vulkan_object_shader_destroy(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);

void vulkan_object_shader_use(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);

void vulkan_object_shader_update_global_state(VulkanContext* context, VulkanObjectShader* shader, int deviceIndex);

void vulkan_object_shader_update_object(VulkanContext* context, VulkanObjectShader* shader,mat4 model,int deviceIndex);