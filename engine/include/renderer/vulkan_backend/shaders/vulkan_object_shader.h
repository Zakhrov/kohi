#pragma once
#include "../vulkan_types.inl"
#include "../../renderer_types.inl"

b8 vulkan_object_shader_create(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);

void vulkan_object_shader_destroy(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);

void vulkan_object_shader_use(VulkanContext* context, VulkanObjectShader* shader,int deviceIndex);
