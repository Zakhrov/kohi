#pragma once

#include "vulkan_types.inl"

b8 vulkan_result_is_success(VkResult result);

const char* vulkan_result_string(VkResult result,b8 getExtended);