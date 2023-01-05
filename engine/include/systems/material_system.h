#pragma once

#include "../defines.h"

#include "../resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MaterialSystemConfig {
    u32 maxMaterialCount;
} MaterialSystemConfig;

typedef struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH];
    b8 autoRelease;
    vec4 diffuseColour;
    char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
} MaterialConfig;

b8 material_system_initialize(u64* memory_requirement, void* state, MaterialSystemConfig config);
void material_system_shutdown(void* state);

Material* material_system_acquire(const char* name);
Material* material_system_acquire_from_config(MaterialConfig config);
void material_system_release(const char* name);