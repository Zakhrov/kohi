#pragma once

#include "../math/math_types.h"

typedef struct Texture {
    const char* name;
    u32 id;
    u32 width;
    u32 height;
    u8 channelCount;
    b8 hasTransparency;
    u32 generation;
    void* internalData;
} Texture;