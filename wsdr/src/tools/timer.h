#ifndef TIMER_H
#define TIMER_H

#include <time.h> 
#include <sys/time.h>

void time_test_1(void);

void time_test_2(void);

void time_test_3(void);

void timer_start(struct timespec *begin);

void timer_end(struct timespec *begin, double *time_spent);

#endif