#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "helpers.h"

uint64_t timestamp() 
{
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}
