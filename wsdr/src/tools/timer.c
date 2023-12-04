#include <sys/time.h>
#include <string.h>
#include <stdio.h> /* printf */
#include <time.h>  /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>  /* sqrt */

#include "timer.h"

static double work2()
{
    double sum = 0;
    double add = 1;

    int iterations = 100;
    for (int i = 0; i < iterations; i++)
    {
        sum += add;
        add /= 2.0;
    }

    return sum;
}

void time_test_1(void)
{
    double sum = 0;

    // Start measuring time
    clock_t start = clock();

    sum = work2();

    // Stop measuring time and calculate the elapsed time
    clock_t end = clock();

    printf("Result: %f\n", sum);

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time1 measured: %f seconds.\n", elapsed);
}

void time_test_2(void)
{
    struct timespec begin, end;
    double sum = 0;

    // Start measuring time
    clock_gettime(CLOCK_REALTIME, &begin);

    sum = work2();

    // Stop measuring time and calculate the elapsed time
    clock_gettime(CLOCK_REALTIME, &end);

    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds;

    printf("Result: %f\n", sum);

    printf("Time2 measured: %f seconds.\n", elapsed);
}

void time_test_3(void)
{
    struct timespec begin, end;
    double sum = 0;

    // Start measuring time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);

    sum = work2();

    // Stop measuring time and calculate the elapsed time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds;

    printf("Result: %f\n", sum);

    printf("Time3 measured: %f \n", elapsed);
}