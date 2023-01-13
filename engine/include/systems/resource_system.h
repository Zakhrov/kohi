#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "../resources/resource_types.h"
typedef struct ResourceSystemConfig{
    u32 maxLoaderCount;
    // Relative base path for assets
    char* assetBasePath;
}ResourceSystemConfig;

typedef struct ResourceLoader{
    u32 id;
    ResourceType type;
    const char* customType;
    const char* typePath;
    b8(*load)(struct ResourceLoader* self,const char* name, Resource* resource);
    void(*unload)(struct ResourceLoader* self,Resource* resource);

}ResourceLoader;

b8 resource_system_initialize(u64* memory_requirement, void* state, ResourceSystemConfig config);
void resource_system_shutdown(void* state);

KAPI b8 resource_system_register_loader(ResourceLoader loader);

KAPI b8 resource_system_load(const char* name, ResourceType type, Resource* resource);
KAPI b8 resource_system_load_custom(const char* name, const char* custom_type, Resource* resource);

KAPI void resource_system_unload(Resource* resource);

KAPI const char* resource_system_base_path();

#ifdef __cplusplus
}
#endif
