#pragma once
#include "../defines.h"

typedef struct KohiClock{
    f64 startTime;
    f64 elaplsedTime;
}KohiClock;
#ifdef __cplusplus
extern "C"
{
#endif
void clock_update(KohiClock* clock);

// Starts the provided clock. Resets elapsed time.
void clock_start(KohiClock* clock);

// Stops the provided clock. Does not reset elapsed time.
void clock_stop(KohiClock* clock);

#ifdef __cplusplus
}
#endif
