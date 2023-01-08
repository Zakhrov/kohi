#pragma once

#include "../defines.h"

#include "../renderer/renderer_types.inl"



typedef struct GeometrySystemConfig{
    u32 maxGeometryCount;
}GeometrySystemConfig;

#define DEFAULT_GEOMETRY_NAME "default"

typedef struct GeometryConfig{
    u32 vertexCount;
    Vertex3D* vertices;
    u32 indexCount;
    u32* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char materialName[MATERIAL_NAME_MAX_LENGTH];
}GeometryConfig;

b8 geometry_system_initialize(u64* memory_requirement, void* state, GeometrySystemConfig config);
void geometry_system_shutdown(void* state);


/**
 * @brief Acquires an existing geometry by id.
 * 
 * @param id The geometry identifier to acquire by.
 * @return A pointer to the acquired geometry or nullptr if failed.
 */
Geometry* geometry_system_acquire_by_id(u32 id);

/**
 * @brief Registers and acquires a new geometry using the given config.
 * 
 * @param config The geometry configuration.
 * @param auto_release Indicates if the acquired geometry should be unloaded when its reference count reaches 0.
 * @return A pointer to the acquired geometry or nullptr if failed. 
 */
Geometry* geometry_system_acquire_from_config(GeometryConfig config, b8 auto_release);

/**
 * @brief Releases a reference to the provided geometry.
 * 
 * @param geometry The geometry to be released.
 */
void geometry_system_release(Geometry* geometry);

/**
 * @brief Obtains a pointer to the default geometry.
 * 
 * @return A pointer to the default geometry. 
 */
Geometry* geometry_system_get_default();

/**
 * @brief Generates configuration for plane geometries given the provided parameters.
 * NOTE: vertex and index arrays are dynamically allocated and should be freed upon object disposal.
 * Thus, this should not be considered production code.
 * 
 * @param width The overall width of the plane. Must be non-zero.
 * @param height The overall height of the plane. Must be non-zero.
 * @param x_segment_count The number of segments along the x-axis in the plane. Must be non-zero.
 * @param y_segment_count The number of segments along the y-axis in the plane. Must be non-zero.
 * @param tile_x The number of times the texture should tile across the plane on the x-axis. Must be non-zero.
 * @param tile_y The number of times the texture should tile across the plane on the y-axis. Must be non-zero.
 * @param name The name of the generated geometry.
 * @param material_name The name of the material to be used.
 * @return A geometry configuration which can then be fed into geometry_system_acquire_from_config().
 */
GeometryConfig geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y, const char* name, const char* material_name);