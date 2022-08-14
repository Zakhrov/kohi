#include "clock.h"

#include "../platform/platform.h"

void clock_update(KohiClock* clock){
    if (clock->startTime != 0) {
        clock->elaplsedTime = platform_get_absolute_time() - clock->startTime;
    }

}

// Starts the provided clock. Resets elapsed time.
void clock_start(KohiClock* clock){
    clock->startTime = platform_get_absolute_time();
    clock->elaplsedTime = 0;

}

// Stops the provided clock. Does not reset elapsed time.
void clock_stop(KohiClock* clock){
    clock->startTime = 0;

}