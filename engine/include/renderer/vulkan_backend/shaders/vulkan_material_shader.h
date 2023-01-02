#pragma once
#include "../vulkan_types.inl"
#include "../../renderer_types.inl"

b8 vulkan_material_shader_create(VulkanContext* context, Texture* defaultDiffuse, VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_destroy(VulkanContext* context, VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_use(VulkanContext* context, VulkanMaterialShader* shader,int deviceIndex);

void vulkan_material_shader_update_global_state(VulkanContext* context, VulkanMaterialShader* shader, f64 deltaTime, int deviceIndex);

void vulkan_material_shader_update_object(VulkanContext* context, VulkanMaterialShader* shader,GeometryRenderData data,int deviceIndex);

b8 vulkan_material_shader_acquire_resources(VulkanContext* context, VulkanMaterialShader* shader, u32* outObjectId, int deviceIndex);
void vulkan_material_shader_release_resources(VulkanContext* context, VulkanMaterialShader* shader, u32 objectId, int deviceIndex);