#pragma once

#include "../math/math_types.h"

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
typedef struct Material {
    u32 id;
    u32 generation;
    u32 internalId;
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuseColour;
    TextureMap diffuseMap;
} Material;