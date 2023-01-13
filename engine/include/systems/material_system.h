#pragma once

#include "../defines.h"

#include "../resources/resource_types.h"
#ifdef __cplusplus
extern "C"
{
#endif




#define DEFAULT_MATERIAL_NAME "default"

typedef struct MaterialSystemConfig {
    u32 maxMaterialCount;
} MaterialSystemConfig;



b8 material_system_initialize(u64* memory_requirement, void* state, MaterialSystemConfig config);
void material_system_shutdown(void* state);

Material* material_system_acquire(const char* name);
Material* material_system_acquire_from_config(MaterialConfig config);
void material_system_release(const char* name);
Material* material_system_get_default();


#ifdef __cplusplus
}
#endif