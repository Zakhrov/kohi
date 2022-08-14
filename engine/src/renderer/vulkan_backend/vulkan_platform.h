#pragma once
#include "../../defines.h"
#ifdef __cplusplus
#include <vector>
extern "C"
{
#endif
void platform_get_required_extension_names(std::vector<const char*>* extensions); 
#ifdef __cplusplus
}
#endif
