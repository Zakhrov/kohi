#include "core/logger.h"
#include "core/kstring.h"
#include "memory/kmemory.h"

#include "systems/geometry_system.h"
#include "systems/material_system.h"
#include "renderer/renderer_frontend.h"


typedef struct GeometryReference{
    u64 referenceCount;
    Geometry geometry;
    b8 autoRelease;
} GeometryReference;

typedef struct GeometrySystemState{
    GeometrySystemConfig config;
    Geometry defaultGeometry;
    GeometryReference* registeredGeometries;
}GeometrySystemState;

static GeometrySystemState* statePtr = 0;

b8 create_default_geometry(GeometrySystemState* state);
b8 create_geometry(GeometrySystemState* state, GeometryConfig config, Geometry* g);
void destroy_geometry(GeometrySystemState* state, Geometry* g);


b8 geometry_system_initialize(u64* memory_requirement, void* state, GeometrySystemConfig config){

        if (config.maxGeometryCount == 0) {
        KFATAL("geometry_system_initialize - config.max_geometry_count must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 struct_requirement = sizeof(GeometrySystemState);
    u64 array_requirement = sizeof(Geometry) * config.maxGeometryCount;
    *memory_requirement = struct_requirement + array_requirement;

    if (!state) {
        return true;
    }

    statePtr = state;
    statePtr->config = config;

    // The array block is after the state. Already allocated, so just set the pointer.
    void* array_block = state + struct_requirement;
    statePtr->registeredGeometries = array_block;

    // Invalidate all geometries in the array.
    u32 count = statePtr->config.maxGeometryCount;
    for (u32 i = 0; i < count; ++i) {
        statePtr->registeredGeometries[i].geometry.id = INVALID_ID;
        statePtr->registeredGeometries[i].geometry.internalId = INVALID_ID;
        statePtr->registeredGeometries[i].geometry.generation = INVALID_ID;
    }

    if (!create_default_geometry(statePtr)) {
        KFATAL("Failed to create default geometry. Application cannot continue.");
        return false;
    }

    return true;

}
void geometry_system_shutdown(void* state){
    // NOTE: Nothing to do here

}

Geometry* geometry_system_acquire_by_id(u32 id){
        if (id != INVALID_ID && statePtr->registeredGeometries[id].geometry.id != INVALID_ID) {
        statePtr->registeredGeometries[id].referenceCount++;
        return &statePtr->registeredGeometries[id].geometry;
    }

    // NOTE: Should return default geometry instead?
    KERROR("geometry_system_acquire_by_id cannot load invalid geometry id. Returning nullptr.");
    return 0;
}

Geometry* geometry_system_acquire_from_config(GeometryConfig config, b8 auto_release){
    Geometry* g = 0;
    for (u32 i = 0; i < statePtr->config.maxGeometryCount; ++i) {
        if (statePtr->registeredGeometries[i].geometry.id == INVALID_ID) {
            // Found empty slot.
            statePtr->registeredGeometries[i].autoRelease = auto_release;
            statePtr->registeredGeometries[i].referenceCount = 1;
            g = &statePtr->registeredGeometries[i].geometry;
            g->id = i;
            break;
        }
    }

    if (!g) {
        KERROR("Unable to obtain free slot for geometry. Adjust configuration to allow more space. Returning nullptr.");
        return 0;
    }

    if (!create_geometry(statePtr, config, g)) {
        KERROR("Failed to create geometry. Returning nullptr.");
        return 0;
    }

    return g;
}
void geometry_system_release(Geometry* geometry){
     if (geometry && geometry->id != INVALID_ID) {
        GeometryReference* ref = &statePtr->registeredGeometries[geometry->id];

        // Take a copy of the id;
        u32 id = geometry->id;
        if (ref->geometry.id == geometry->id) {
            if (ref->referenceCount > 0) {
                ref->referenceCount--;
            }

            // Also blanks out the geometry id.
            if (ref->referenceCount < 1 && ref->autoRelease) {
                destroy_geometry(statePtr, &ref->geometry);
                ref->referenceCount = 0;
                ref->autoRelease = false;
            }
        } else {
            KFATAL("Geometry id mismatch. Check registration logic, as this should never occur.");
        }
        return;
    }

    KWARN("geometry_system_acquire_by_id cannot release invalid geometry id. Nothing was done.");

}
Geometry* geometry_system_get_default(){
    if (statePtr) {
        return &statePtr->defaultGeometry;
    }

    KFATAL("geometry_system_get_default called before system was initialized. Returning nullptr.");
    return 0;

}
GeometryConfig geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y, const char* name, const char* material_name){

    if (width == 0) {
        KWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0) {
        KWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }
    if (x_segment_count < 1) {
        KWARN("x_segment_count must be a positive number. Defaulting to one.");
        x_segment_count = 1;
    }
    if (y_segment_count < 1) {
        KWARN("y_segment_count must be a positive number. Defaulting to one.");
        y_segment_count = 1;
    }

    if (tile_x == 0) {
        KWARN("tile_x must be nonzero. Defaulting to one.");
        tile_x = 1.0f;
    }
    if (tile_y == 0) {
        KWARN("tile_y must be nonzero. Defaulting to one.");
        tile_y = 1.0f;
    }

    GeometryConfig config;
    config.vertexCount = x_segment_count * y_segment_count * 4;  // 4 verts per segment
    config.vertices = kallocate(sizeof(Vertex3D) * config.vertexCount, MEMORY_TAG_ARRAY);
    config.indexCount = x_segment_count * y_segment_count * 6;  // 6 indices per segment
    config.indices = kallocate(sizeof(u32) * config.indexCount, MEMORY_TAG_ARRAY);

    // TODO: This generates extra vertices, but we can always deduplicate them later.
    f32 seg_width = width / x_segment_count;
    f32 seg_height = height / y_segment_count;
    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;
    for (u32 y = 0; y < y_segment_count; ++y) {
        for (u32 x = 0; x < x_segment_count; ++x) {
            // Generate vertices
            f32 min_x = (x * seg_width) - half_width;
            f32 min_y = (y * seg_height) - half_height;
            f32 max_x = min_x + seg_width;
            f32 max_y = min_y + seg_height;
            f32 min_uvx = (x / (f32)x_segment_count) * tile_x;
            f32 min_uvy = (y / (f32)y_segment_count) * tile_y;
            f32 max_uvx = ((x + 1) / (f32)x_segment_count) * tile_x;
            f32 max_uvy = ((y + 1) / (f32)y_segment_count) * tile_y;

            u32 v_offset = ((y * x_segment_count) + x) * 4;
            Vertex3D* v0 = &config.vertices[v_offset + 0];
            Vertex3D* v1 = &config.vertices[v_offset + 1];
            Vertex3D* v2 = &config.vertices[v_offset + 2];
            Vertex3D* v3 = &config.vertices[v_offset + 3];

            v0->position.x = min_x;
            v0->position.y = min_y;
            v0->texcoord.x = min_uvx;
            v0->texcoord.y = min_uvy;

            v1->position.x = max_x;
            v1->position.y = max_y;
            v1->texcoord.x = max_uvx;
            v1->texcoord.y = max_uvy;

            v2->position.x = min_x;
            v2->position.y = max_y;
            v2->texcoord.x = min_uvx;
            v2->texcoord.y = max_uvy;

            v3->position.x = max_x;
            v3->position.y = min_y;
            v3->texcoord.x = max_uvx;
            v3->texcoord.y = min_uvy;

            // Generate indices
            u32 i_offset = ((y * x_segment_count) + x) * 6;
            config.indices[i_offset + 0] = v_offset + 0;
            config.indices[i_offset + 1] = v_offset + 1;
            config.indices[i_offset + 2] = v_offset + 2;
            config.indices[i_offset + 3] = v_offset + 0;
            config.indices[i_offset + 4] = v_offset + 3;
            config.indices[i_offset + 5] = v_offset + 1;
        }
    }

    if (name && string_length(name) > 0) {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0) {
        string_ncopy(config.materialName, material_name, MATERIAL_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.materialName, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;

}
b8 create_default_geometry(GeometrySystemState* state){
    Vertex3D verts[4];
    kzero_memory(verts, sizeof(Vertex3D) * 4);

    const f32 f = 10.0f;

    verts[0].position.x = -0.5 * f;  // 0    3
    verts[0].position.y = -0.5 * f;  //
    verts[0].texcoord.x = 0.0f;      //
    verts[0].texcoord.y = 0.0f;      // 2    1

    verts[1].position.y = 0.5 * f;
    verts[1].position.x = 0.5 * f;
    verts[1].texcoord.x = 1.0f;
    verts[1].texcoord.y = 1.0f;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[2].texcoord.x = 0.0f;
    verts[2].texcoord.y = 1.0f;

    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x = 1.0f;
    verts[3].texcoord.y = 0.0f;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    // Send the geometry off to the renderer to be uploaded to the GPU.
    if (!renderer_create_geometry(&state->defaultGeometry, 4, verts, 6, indices)) {
        KFATAL("Failed to create default geometry. Application cannot continue.");
        return false;
    }

    // Acquire the default material.
    state->defaultGeometry.material = material_system_get_default();

    return true;

}
b8 create_geometry(GeometrySystemState* state, GeometryConfig config, Geometry* g){
        // Send the geometry off to the renderer to be uploaded to the GPU.
    if (!renderer_create_geometry(g, config.vertexCount, config.vertices, config.indexCount, config.indices)) {
        // Invalidate the entry.
        state->registeredGeometries[g->id].referenceCount = 0;
        state->registeredGeometries[g->id].autoRelease = false;
        g->id = INVALID_ID;
        g->generation = INVALID_ID;
        g->internalId = INVALID_ID;

        return false;
    }

    // Acquire the material
    if (string_length(config.materialName) > 0) {
        g->material = material_system_acquire(config.materialName);
        if (!g->material) {
            g->material = material_system_get_default();
        }
    }

    return true;

}
void destroy_geometry(GeometrySystemState* state, Geometry* g){
    renderer_destroy_geometry(g);
    g->internalId = INVALID_ID;
    g->generation = INVALID_ID;
    g->id = INVALID_ID;
    string_empty(g->name);
     // Release the material.
    if (g->material && string_length(g->material->name) > 0) {
        material_system_release(g->material->name);
        g->material = 0;
    }

}