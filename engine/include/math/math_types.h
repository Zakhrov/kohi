#pragma once
#include "../defines.h"

typedef union vec2_u {
    f32 elements[2];
    struct {
        union{
            f32 x,r,s,u;
        };
        union{
            f32 y,g,t,v;
        };
    };
} vec2;
typedef struct vec3_u {
    union {
        // An array of x, y, z
        f32 elements[3];
        struct {
            union {
                // The first element.
                f32 x, r, s, u;
            };
            union {
                // The second element.
                f32 y, g, t, v;
            };
            union {
                // The third element.
                f32 z, b, p, w;
            };
        };
    };
} vec3;

typedef union vec4_u {
#if defined(KUSE_SIMD)
    // Used for SIMD operations.
    alignas(16) __m128 data;
#endif
    // An array of x, y, z, w
    f32 elements[4];
    union {
        struct {
            union {
                // The first element.
                f32 x, r, s;
            };
            union {
                // The second element.
                f32 y, g, t;
            };
            union {
                // The third element.
                f32 z, b, p;
            };
            union {
                // The fourth element.
                f32 w, a, q;
            };
        };
    };
} vec4;

typedef vec4 quat;

typedef union mat4_u {
    f32 data[16];
} mat4;

typedef struct Vertex3D {
    vec3 position;
    vec2 texcoord;
} Vertex3D;
