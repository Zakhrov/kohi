#pragma once
#include "../vulkan_types.inl"
#include "../../renderer_types.inl"

b8 vulkan_material_shader_create(VulkanContext* context,  VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_destroy(VulkanContext* context, VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_use(VulkanContext* context, VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_update_global_state(VulkanContext* context, VulkanMaterialShader* shader, f64 deltaTime, int deviceIndex);

void vulkan_material_shader_set_model(VulkanContext* context, VulkanMaterialShader* shader, mat4 model, int deviceIndex);
void vulkan_material_shader_apply_material(VulkanContext* context, VulkanMaterialShader* shader, Material* material, int deviceIndex);

b8 vulkan_material_shader_acquire_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material, int deviceIndex);
void vulkan_material_shader_release_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material, int deviceIndex);
