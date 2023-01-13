#include "systems/resource_system.h"

#include "core/logger.h"
#include "core/kstring.h"

#include "resources/loaders/binary_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"


typedef struct ResourceSystemState{
    ResourceSystemConfig config;
    ResourceLoader* registeredLoaders;
}ResourceSystemState;

static ResourceSystemState* statePtr = 0;

b8 load_resource(const char* name, ResourceLoader* loader,Resource* resource);
b8 resource_system_initialize(u64* memory_requirement, void* state, ResourceSystemConfig config){
    KDEBUG("Initializing Resource Subsystem");
    if(config.maxLoaderCount == 0){
        KFATAL("resource_system_initialize failed because config.max_loader_count==0.");
        return false;

    }
    *memory_requirement = sizeof(ResourceSystemState) + (sizeof(ResourceLoader) * config.maxLoaderCount);

    if (!state) {
        return true;
    }

    statePtr = state;
    statePtr->config = config;

    void* array_block = state + sizeof(ResourceSystemState) * config.maxLoaderCount;
    statePtr->registeredLoaders = array_block;

    // Invalidate all loaders
    u32 count = config.maxLoaderCount;
    for (u32 i = 0; i < count; ++i) {
        statePtr->registeredLoaders[i].id = INVALID_ID;
    }

    // NOTE: Auto register known loader types
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());

    KINFO("Resource system initialized with base path %s",config.assetBasePath);

}
void resource_system_shutdown(void* state){
    if(statePtr){
        statePtr = 0;
    }

}

b8 resource_system_register_loader(ResourceLoader loader){
        if (statePtr) {
        u32 count = statePtr->config.maxLoaderCount;
        // Ensure no loaders for the given type already exist
        for (u32 i = 0; i < count; ++i) {
            ResourceLoader* l = &statePtr->registeredLoaders[i];
            if (l->id != INVALID_ID) {
                if (l->type == loader.type) {
                    KERROR("resource_system_register_loader - Loader of type %d already exists and will not be registered.", loader.type);
                    return false;
                } else if (loader.customType && string_length(loader.customType) > 0 && strings_equali(l->customType, loader.customType)) {
                    KERROR("resource_system_register_loader - Loader of custom type %s already exists and will not be registered.", loader.customType);
                    return false;
                }
            }
        }
        for (u32 i = 0; i < count; ++i) {
            if (statePtr->registeredLoaders[i].id == INVALID_ID) {
                statePtr->registeredLoaders[i] = loader;
                statePtr->registeredLoaders[i].id = i;
                KTRACE("Loader registered.");
                return true;
            }
        }
    }

    return false;



}

b8 resource_system_load(const char* name, ResourceType type, Resource* resource){
    KDEBUG("Loading Resource %s",name);
    if(statePtr && type != RESOURCE_TYPE_CUSTOM){
        resource->name = name;
        // select loader
        u32 count = statePtr->config.maxLoaderCount;
        for(u32 i = 0; i< count; i++){
            ResourceLoader* l = &statePtr->registeredLoaders[i];
            if(l->id != INVALID_ID && l->type == type){
                if(type == RESOURCE_TYPE_BINARY){
                    KDEBUG("Loading Binary Resource %s",name);
                }
                return load_resource(name,l,resource);
            }
        }
    }
    resource->loaderId = INVALID_ID;
    KERROR("resource_system_load No loader for type %d was found",type);
    return false;

}
b8 resource_system_load_custom(const char* name, const char* custom_type, Resource* resource){
        if(statePtr && custom_type && string_length(custom_type) > 0){
        // select loader
        u32 count = statePtr->config.maxLoaderCount;
        for(u32 i = 0; i< count; i++){
            ResourceLoader* l = &statePtr->registeredLoaders[i];
            if(l->id != INVALID_ID && l->type == RESOURCE_TYPE_CUSTOM && strings_equali(l->customType,custom_type)){
                return load_resource(name,l,resource);
            }
        }
    }
    resource->loaderId = INVALID_ID;
    KERROR("resource_system_load No loader for type %s was found",custom_type);
    return false;

}

void resource_system_unload(Resource* resource){
    if(statePtr && resource){
        if(resource->loaderId != INVALID_ID){
            ResourceLoader* l = &statePtr->registeredLoaders[resource->loaderId];
            if(l->id != INVALID_ID){
                l->unload(l,resource);
            }
        }
    }

}

const char* resource_system_base_path(){
    if(statePtr){
        return statePtr->config.assetBasePath;
    }
    KERROR("resource_system_base_path called before initialization, returning empty string");
    return "";

}
b8 load_resource(const char* name, ResourceLoader* loader,Resource* resource){
    if(!name || !loader || !loader->load || !resource){
        resource->loaderId = INVALID_ID;
        return false;
    }
    resource->loaderId = loader->id;
    return loader->load(loader,name,resource);
    

}