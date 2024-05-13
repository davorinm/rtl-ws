#include <sys/time.h>
#include <string.h>
#include <stdio.h> /* printf */
#include <time.h>  /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>  /* sqrt */

#include "timer.h"
#include "helpers.h"

#define BILLION 1000000000.0

static double work2()
{
    double sum = 0;
    double add = 1;

    int iterations = 10000;
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

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Time1 measured: %f ms to execute \n", cpu_time_used);
    printf("Time1 measured: %lu, %lu, %lu \n", end, start, CLOCKS_PER_SEC);
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

    printf("Result: %f\n", sum);

    printf("Time2 measured: %lds %ldns \n", seconds, nanoseconds);
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

    printf("Result: %f\n", sum);

    printf("Time3 measured: %lds %ldns \n", seconds, nanoseconds);
}

void timer_start(struct timespec *begin)
{
    clock_gettime(CLOCK_REALTIME, begin);
}

void timer_end(struct timespec *begin, double *time_spent)
{
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);

    // time_spent = end - start
    *time_spent = (end.tv_sec - begin->tv_sec) + ((end.tv_nsec - begin->tv_nsec) / BILLION);
}

#define N 10

double gathering_avg = 0;
double processing_avg = 0;
double payload_avg = 0;
double sending_avg = 0;
double audio_avg = 0;
double spectrum_avg = 0;

double timer_avg(char *key) {
    if (strcmp(key, "GATHERING") == 0)
    {
        return gathering_avg;
    }
    else if (strcmp(key, "PROCESSING") == 0)
    {
        return processing_avg;
    }
    else if (strcmp(key, "PAYLOAD") == 0)
    {
        return payload_avg;
    }
    else if (strcmp(key, "SENDING_WS") == 0)
    {
        return sending_avg;
    }
    else if (strcmp(key, "AUDIO") == 0)
    {
        return audio_avg;
    }
    else if (strcmp(key, "SPECTRUM") == 0)
    {
        return spectrum_avg;
    }

    return 0;
}

void timer_log(char *key, double value)
{
    if (strcmp(key, "GATHERING") == 0)
    {
        gathering_avg -= gathering_avg / N;
        gathering_avg += value / N;
    }
    else if (strcmp(key, "PROCESSING") == 0)
    {
        processing_avg -= processing_avg / N;
        processing_avg += value / N;
    }
    else if (strcmp(key, "PAYLOAD") == 0)
    {
        payload_avg -= payload_avg / N;
        payload_avg += value / N;
    }
    else if (strcmp(key, "SENDING_WS") == 0)
    {
        sending_avg -= sending_avg / N;
        sending_avg += value / N;
    }
    else if (strcmp(key, "AUDIO") == 0)
    {
        audio_avg -= audio_avg / N;
        audio_avg += value / N;
    }
    else if (strcmp(key, "SPECTRUM") == 0)
    {
        spectrum_avg -= spectrum_avg / N;
        spectrum_avg += value / N;
    }
}