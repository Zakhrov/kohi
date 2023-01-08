#pragma once
#include "../defines.h"
#include "../math/math_types.h"
#include "../resources/resource_types.h"
typedef enum RendererBackendType {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
}RendererBackendType;


typedef struct GlobalUniformObject{
    mat4 projection;
    mat4 view;
    mat4 reserved1; // 64 bytes reserved for padding. UBOs should be 256 bytes long
    mat4 reserved2; // 64 bytes reserved for padding. UBOs should be 256 bytes long

}GlobalUniformObject;

typedef struct MaterialUniformObject{
    vec4 diffuseColor;
    vec4 reserved1; // 16 bytes reserved for padding
    vec4 reserved2; // 16 bytes reserved for padding
    vec4 reserved3; // 16 bytes reserved for padding

}MaterialUniformObject;
typedef struct GeometryRenderData{
    Geometry* geometry;
    mat4 model;
}GeometryRenderData;

typedef struct RendererBackend{
    struct PlatformState* platformState;
    u64 frameNumber;
    b8(*initialize)(struct RendererBackend* backend, const char* applicationName);
    void(*shutdown)(struct RendererBackend* backend);
    void(*resized)(struct RendererBackend* backend, u16 width, u16 height);
    void(*update_global_state)(struct RendererBackend* backend,mat4 projection,mat4 view, vec3 viewPosition,vec4 ambientColor,i32 mode);
    void(*draw_geometry)(struct RendererBackend* backend,GeometryRenderData data);
    b8(*begin_frame)(struct RendererBackend* backend, f64 deltaTime);
    b8(*end_frame)(struct RendererBackend* backend, f64 deltaTime);
    void(*create_texture)(const u8* pixels, Texture* texture);
    void(*destroy_texture)(Texture* texture);
    b8(*create_material)(Material* material);
    void(*destroy_material)(Material* material);
    b8(*create_geometry)(Geometry* geometry,u32 vertexCount,const Vertex3D* vertices, u32 indexCount,const u32* indices);
    void(*destroy_geometry)(Geometry* geometry);

}RendererBackend;

typedef struct RenderPacket{
    f32 deltaTime;
    u32 geometryCount;
    GeometryRenderData* geometries;
}RenderPacket;
