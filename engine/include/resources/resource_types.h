#pragma once

#include "../math/math_types.h"

typedef enum ResourceType{
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    RESOURCE_TYPE_CUSTOM
}ResourceType;

typedef struct Resource{
    u32 loaderId;
    const char* name;
    char* fullPath;
    u64 dataSize;
    void* data;
}Resource;

typedef struct ImageResourceData{
    u8 channelCount;
    u32 width;
    u32 height;
    u8* pixels;
}ImageResourceData;

#define TEXTURE_NAME_MAX_LENGTH 512
typedef struct Texture {
    char name[TEXTURE_NAME_MAX_LENGTH];
    u32 id;
    u32 width;
    u32 height;
    u8 channelCount;
    b8 hasTransparency;
    u32 generation;
    void* internalData;
} Texture;

typedef enum TextureUse {
    TEXTURE_USE_UNKNOWN = 0x00,
    TEXTURE_USE_MAP_DIFFUSE = 0x01
} TextureUse;

typedef struct TextureMap {
    Texture* texture;
    TextureUse textureUse;
} TextureMap;

#define MATERIAL_NAME_MAX_LENGTH 256
typedef struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH];
    b8 autoRelease;
    vec4 diffuseColour;
    char diffuseMapName[TEXTURE_NAME_MAX_LENGTH];
} MaterialConfig;
typedef struct Material {
    u32 id;
    u32 generation;
    u32 internalId;
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuseColour;
    TextureMap diffuseMap;
} Material;

#define GEOMETRY_NAME_MAX_LENGTH 256

typedef struct Geometry {
    char name[GEOMETRY_NAME_MAX_LENGTH];
    u32 id;
    u32 internalId;
    u32 generation;
    Material* material;
    

}Geometry;
