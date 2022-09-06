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
    mat4 reserved1;
    mat4 reserved2;

}GlobalUniformObject;

typedef struct RendererBackend{
    struct PlatformState* platformState;
    u64 frameNumber;
    b8(*initialize)(struct RendererBackend* backend, const char* applicationName);
    void(*shutdown)(struct RendererBackend* backend);
    void(*resized)(struct RendererBackend* backend, u16 width, u16 height);
    b8(*begin_frame)(struct RendererBackend* backend, f64 deltaTime);
    b8(*end_frame)(struct RendererBackend* backend, f64 deltaTime);

}RendererBackend;