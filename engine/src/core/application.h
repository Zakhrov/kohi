#pragma once

#include "../defines.h"
#include "logger.h"

struct Game;
typedef struct ApplicationConfig{
    i16 startX;
    i16 startY;
    i16 startWidth;
    i16 startHeight;
    char* name;

}ApplicationConfig;

#ifdef __cplusplus
extern "C"
{
#endif

KAPI b8 application_create(struct Game* gameInstance);

KAPI b8 application_run();

#ifdef __cplusplus
}
#endif