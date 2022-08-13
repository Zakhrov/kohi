#pragma once

#include "../defines.h"
#include "logger.h"

typedef struct ApplicationConfig{
    i16 startX;
    i16 startY;
    i16 startWidth;
    i16 startHeight;
    char* name;

}ApplicationConfig;

KAPI b8 application_create(ApplicationConfig* config);

KAPI b8 application_run();