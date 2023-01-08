#include "systems/material_system.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "containers/hashtable.h"
#include "math/kmath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

// TODO: temp: resource system
#include "platform/filesystem.h"
// end temp


typedef struct MaterialSystemState {
    MaterialSystemConfig config;

    Material defaultMaterial;

    // Array of registered materials.
    Material* registeredMaterials;

    // Hashtable for material lookups.
    HashTable registeredMaterialTable;
} MaterialSystemState;

typedef struct MaterialReference {
    u64 referenceCount;
    u32 handle;
    b8 autoRelease;
} MaterialReference;

static MaterialSystemState* statePtr = 0;

b8 create_defaultMaterial(MaterialSystemState* state);
b8 load_material(MaterialConfig config, Material* m);
void destroy_material(Material* m);
b8 load_configuration_file(const char* path, MaterialConfig* config);


b8 material_system_initialize(u64* memory_requirement, void* state, MaterialSystemConfig config){
    if(config.maxMaterialCount == 0){
        KFATAL("Material System initialize config.maxMaterialCount should be > 0");
        return false;
    }
    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(MaterialSystemState);
    u64 array_requirement = sizeof(Material) * config.maxMaterialCount;
    u64 hashtable_requirement = sizeof(MaterialReference) * config.maxMaterialCount;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    if (!state) {
        return true;
    }

    statePtr = state;
    statePtr->config = config;

    // The array block is after the state. Already allocated, so just set the pointer.
    void* array_block = state + struct_requirement;
    statePtr->registeredMaterials = array_block;

    // Hashtable block is after array.
    void* hashtable_block = array_block + array_requirement;

    // Create a hashtable for material lookups.
    hashtable_create(sizeof(MaterialReference), config.maxMaterialCount, hashtable_block, false, &statePtr->registeredMaterialTable);

    // Fill the hashtable with invalid references to use as a default.
    MaterialReference invalid_ref;
    invalid_ref.autoRelease = false;
    invalid_ref.handle = INVALID_ID;  // Primary reason for needing default values.
    invalid_ref.referenceCount = 0;
    hashtable_fill(&statePtr->registeredMaterialTable, &invalid_ref);

    // Invalidate all materials in the array.
    u32 count = statePtr->config.maxMaterialCount;
    for (u32 i = 0; i < count; ++i) {
        statePtr->registeredMaterials[i].id = INVALID_ID;
        statePtr->registeredMaterials[i].generation = INVALID_ID;
        statePtr->registeredMaterials[i].internalId = INVALID_ID;
    }

    if (!create_defaultMaterial(statePtr)) {
        KFATAL("Failed to create default material. Application cannot continue.");
        return false;
    }
    KINFO("Material System Initialized");

    return true;


}
void material_system_shutdown(void* state){
    MaterialSystemState* s = (MaterialSystemState*)state;
    if (s) {
        // Invalidate all materials in the array.
        u32 count = s->config.maxMaterialCount;
        for (u32 i = 0; i < count; ++i) {
            if (s->registeredMaterials[i].id != INVALID_ID) {
                destroy_material(&s->registeredMaterials[i]);
            }
        }

        // Destroy the default material.
        destroy_material(&s->defaultMaterial);
    }

    statePtr = 0;

}

Material* material_system_acquire(const char* name){
    // Load the given material configuration from disk.
    MaterialConfig config;

    // Load file from disk
    // TODO: Should be able to be located anywhere.
    char* format_str = "../assets/materials/%s.%s";
    char full_file_path[512];

    // TODO: try different extensions
    string_format(full_file_path, format_str, name, "kmt");
    
    if (!load_configuration_file(full_file_path, &config)) {
        KERROR("Failed to load material file: '%s'. Null pointer will be returned.", full_file_path);
        return 0;
    }

    // Now acquire from loaded config.
    return material_system_acquire_from_config(config);

}
Material* material_system_acquire_from_config(MaterialConfig config){

    // Return default material.
    if (strings_equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &statePtr->defaultMaterial;
    }

    MaterialReference ref;
    if (statePtr && hashtable_get(&statePtr->registeredMaterialTable, config.name, &ref)) {
        // This can only be changed the first time a material is loaded.
        if (ref.referenceCount == 0) {
            ref.autoRelease = config.autoRelease;
        }
        ref.referenceCount++;
        if (ref.handle == INVALID_ID) {
            // This means no material exists here. Find a free index first.
            u32 count = statePtr->config.maxMaterialCount;
            Material* m = 0;
            for (u32 i = 0; i < count; ++i) {
                if (statePtr->registeredMaterials[i].id == INVALID_ID) {
                    // A free slot has been found. Use its index as the handle.
                    ref.handle = i;
                    m = &statePtr->registeredMaterials[i];
                    break;
                }
            }

            // Make sure an empty slot was actually found.
            if (!m || ref.handle == INVALID_ID) {
                KFATAL("material_system_acquire - Material system cannot hold anymore materials. Adjust configuration to allow more.");
                return 0;
            }

            // Create new material.
            if (!load_material(config, m)) {
                KERROR("Failed to load material '%s'.", config.name);
                return 0;
            }

            if (m->generation == INVALID_ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // Also use the handle as the material id.
            m->id = ref.handle;
            KTRACE("Material '%s' does not yet exist. Created, and ref_count is now %i.", config.name, ref.referenceCount);
        } else {
            KTRACE("Material '%s' already exists, ref_count increased to %i.", config.name, ref.referenceCount);
        }

        // Update the entry.
        hashtable_set(&statePtr->registeredMaterialTable, config.name, &ref);
        return &statePtr->registeredMaterials[ref.handle];
    }

    // NOTE: This would only happen in the event something went wrong with the state.
    KERROR("material_system_acquire_from_config failed to acquire material '%s'. Null pointer will be returned.", config.name);
    return 0;

}
void material_system_release(const char* name){
    // Ignore release requests for the default material.
    if (strings_equali(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }
    MaterialReference ref;
    if (statePtr && hashtable_get(&statePtr->registeredMaterialTable, name, &ref)) {
        if (ref.referenceCount == 0) {
            KWARN("Tried to release non-existent material: '%s'", name);
            return;
        }
        ref.referenceCount--;
        if (ref.referenceCount == 0 && ref.autoRelease) {
            Material* m = &statePtr->registeredMaterials[ref.handle];

            // Destroy/reset material.
            destroy_material(m);

            // Reset the reference.
            ref.handle = INVALID_ID;
            ref.autoRelease = false;
            KTRACE("Released material '%s'., Material unloaded because reference count=0 and auto_release=true.", name);
        } else {
            KTRACE("Released material '%s', now has a reference count of '%i' (auto_release=%s).", name, ref.referenceCount, ref.autoRelease ? "true" : "false");
        }

        // Update the entry.
        hashtable_set(&statePtr->registeredMaterialTable, name, &ref);
    } else {
        KERROR("material_system_release failed to release material '%s'.", name);
    }

}

b8 load_material(MaterialConfig config, Material* m){
    kzero_memory(m, sizeof(Material));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    // Diffuse colour
    m->diffuseColour = config.diffuseColour;

    // Diffuse map
    if (string_length(config.diffuseMapName) > 0) {
        m->diffuseMap.textureUse = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuseMap.texture = texture_system_acquire(config.diffuseMapName, true);
        
        if (!m->diffuseMap.texture) {
            KWARN("Unable to load texture '%s' for material '%s', using default.", config.diffuseMapName, m->name);
            m->diffuseMap.texture = texture_system_get_default_texture();
            KTRACE("%d",m->diffuseMap.texture->internalData);
            if(!m->diffuseMap.texture->internalData){
                KFATAL("load_material TextureMap internal data null");
            }
        }
    } else {
        // NOTE: Only set for clarity, as call to kzero_memory above does this already.
        m->diffuseMap.textureUse = TEXTURE_USE_UNKNOWN;
        m->diffuseMap.texture = 0;
    }

    // TODO: other maps

    // Send it off to the renderer to acquire resources.
    if (!renderer_create_material(m)) {
        KERROR("Failed to acquire renderer resources for material '%s'.", m->name);
        return false;
    }

    return true;
}
void destroy_material(Material* m){
    KTRACE("Destroying material '%s'...", m->name);

    // Release texture references.
    if (m->diffuseMap.texture) {
        texture_system_release(m->diffuseMap.texture->name);
    }

    // Release renderer resources.
    renderer_destroy_material(m);

    // Zero it out, invalidate IDs.
    kzero_memory(m, sizeof(Material));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internalId = INVALID_ID;

}

b8 create_defaultMaterial(MaterialSystemState* state){
    kzero_memory(&state->defaultMaterial, sizeof(Material));
    state->defaultMaterial.id = INVALID_ID;
    state->defaultMaterial.generation = INVALID_ID;
    string_ncopy(state->defaultMaterial.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->defaultMaterial.diffuseColour = vec4_one();  // white
    state->defaultMaterial.diffuseMap.textureUse = TEXTURE_USE_MAP_DIFFUSE;
    state->defaultMaterial.diffuseMap.texture = texture_system_get_default_texture();
    

    if (!renderer_create_material(&state->defaultMaterial)) {
        KFATAL("Failed to acquire renderer resources for default texture. Application cannot continue.");
        return false;
    }
    hashtable_set(&state->registeredMaterialTable,DEFAULT_MATERIAL_NAME,&state->defaultMaterial);

    return true;

}


b8 load_configuration_file(const char* path, MaterialConfig* config){
    FileHandle f;
    if(!filesystem_open(path,FILE_MODE_READ,false,&f)){
        KERROR("load_configuration_file Could not open file '%s' ",path);
        return false;
    }
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
            KWARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", path, lineNumber);
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
            string_ncopy(config->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(config->diffuseMapName, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_colour")) {
            // Parse the colour
            if (!string_to_vec4(trimmed_value, &config->diffuseColour)) {
                KWARN("Error parsing diffuse_colour in file '%s'. Using default of white instead.", path);
                config->diffuseColour = vec4_one();  // white
            }
        }

        // TODO: more fields.

        // Clear the line buffer.
        kzero_memory(lineBuffer, sizeof(char) * 512);
        lineNumber++;
    }

    filesystem_close(&f);
    KDEBUG("MATERIAL CONFIG NAME %s",config->name);

    return true;

 }
Material* material_system_get_default(){
    if(statePtr){
        return &statePtr->defaultMaterial;
    }
    KFATAL("material_system_get_default called before system is initialized.");
    return 0;
}
