#pragma once
#include "../defines.h"
#include "../math/math_types.h"
typedef enum RendererBackendType {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
}RendererBackendType;

typedef struct RenderPacket{
    f32 deltaTime;
}RenderPacket;

typedef struct GlobalUniformObject{
    mat4 projection;
    mat4 view;
    mat4 reserved1; // reserved due to NVIDIA requirement UBOs should be 256 bytes long
    mat4 reserved2; // reserved due to NVIDIA requirement

}GlobalUniformObject;

typedef struct RendererBackend{
    struct PlatformState* platformState;
    u64 frameNumber;
    b8(*initialize)(struct RendererBackend* backend, const char* applicationName);
    void(*shutdown)(struct RendererBackend* backend);
    void(*resized)(struct RendererBackend* backend, u16 width, u16 height);
    void(*update_global_state)(struct RendererBackend* backend,mat4 projection,mat4 view, vec3 viewPosition,vec4 ambientColor,i32 mode);
    void(*update_object)(struct RendererBackend* backend,mat4 model);
    b8(*begin_frame)(struct RendererBackend* backend, f64 deltaTime);
    b8(*end_frame)(struct RendererBackend* backend, f64 deltaTime);

}RendererBackend;