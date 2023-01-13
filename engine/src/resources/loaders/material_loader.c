#include "resources/loaders/material_loader.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/kmath.h"
#include "platform/filesystem.h"

b8 material_loader_load(ResourceLoader* self,const char* name,Resource* resource){
    if (!self || !name || !resource) {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char fullFilePath[512];
    string_format(fullFilePath,format_str,resource_system_base_path(),self->typePath,name,".kmt");
    FileHandle f;
    if(!filesystem_open(fullFilePath,FILE_MODE_READ,false,&f)){
        KERROR("material_loader_load Could not open file '%s' ",fullFilePath);
        return false;
    }
    // TODO: Should be using an allocator here.
    resource->fullPath = string_duplicate(fullFilePath);

    // TODO: Should be using an allocator here.
    MaterialConfig* resourceData = kallocate(sizeof(MaterialConfig), MEMORY_TAG_MATERIAL_INSTANCE);
    resourceData->autoRelease = true;
    resourceData->diffuseColour = vec4_one();
    resourceData->diffuseMapName[0] = 0;
    string_ncopy(resourceData->name,name,MATERIAL_NAME_MAX_LENGTH);
    char lineBuffer[512] = "";
    char* p = &lineBuffer[0];
    u64 lineLength = 0;
    u32 lineNumber = 1;
    while(filesystem_read_line(&f,511,&p,&lineLength)){

        char* trimmed = string_trim(lineBuffer);
        lineLength = string_length(trimmed);

        if(lineLength < 1 || trimmed[0] == '#'){
            lineNumber++;
            continue;
        }
         // Split into var/value
        i32 equal_index = string_index_of(trimmed, '=');
        if (equal_index == -1) {
            KWARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", fullFilePath, lineNumber);
            lineNumber++;
            continue;
        }
                // Assume a max of 64 characters for the variable name.
        char raw_var_name[64];
        kzero_memory(raw_var_name, sizeof(char) * 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // Assume a max of 511-65 (446) for the max length of the value to account for the variable name and the '='.
        char raw_value[446];
        kzero_memory(raw_value, sizeof(char) * 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1);  // Read the rest of the line
        char* trimmed_value = string_trim(raw_value);

        // Process the variable.
        if (strings_equali(trimmed_var_name, "version")) {
            // TODO: version
        } else if (strings_equali(trimmed_var_name, "name")) {
            string_ncopy(resourceData->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(resourceData->diffuseMapName, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_colour")) {
            // Parse the colour
            if (!string_to_vec4(trimmed_value, &resourceData->diffuseColour)) {
                KWARN("Error parsing diffuse_colour in file '%s'. Using default of white instead.", fullFilePath);
                
            }
        }

        // TODO: more fields.

        // Clear the line buffer.
        kzero_memory(lineBuffer, sizeof(char) * 512);
        lineNumber++;
    }
    filesystem_close(&f);
    KDEBUG("MATERIAL CONFIG NAME %s",resourceData->name);
    resource->data = resourceData;
    resource->dataSize = sizeof(MaterialConfig);
    resource->name = name;
    return true;


}

void material_loader_unload(ResourceLoader* self, Resource* resource) {
    if (!self || !resource) {
        KWARN("image_loader_unload called with nullptr for self or resource.");
        return;
    }

    u32 path_length = string_length(resource->fullPath);
    if (path_length) {
        kfree(resource->fullPath, sizeof(char) * path_length + 1, MEMORY_TAG_STRING);
    }

    if (resource->data) {
        kfree(resource->data, resource->dataSize, MEMORY_TAG_TEXTURE);
        resource->data = 0;
        resource->dataSize = 0;
        resource->loaderId = INVALID_ID;
    }
}

ResourceLoader material_resource_loader_create(){
    ResourceLoader loader;
    loader.type = RESOURCE_TYPE_MATERIAL;
    loader.customType = 0;
    loader.load = material_loader_load;
    loader.unload = material_loader_unload;
    loader.typePath = "materials";
    
    return loader;

}