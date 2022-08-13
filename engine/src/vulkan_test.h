#pragma once

#include "defines.h"

#include "core/logger.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C"
{
#endif
KAPI b8 vulkan_test_init();

KAPI  void vulkan_test_shutdown();

#ifdef __cplusplus
}
#endif