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
    *time_spent = (end.tv_sec - begin->tv_sec) + (end.tv_nsec - begin->tv_nsec) / BILLION;
}

#define DATA_COUNT 100

double gathering_data[DATA_COUNT] = {0};
int gathering_index = 0;
double gathering_avg = 0;

double processing_data[DATA_COUNT] = {0};
int processing_index = 0;
double processing_avg = 0;

double payload_data[DATA_COUNT] = {0};
int payload_index = 0;
double payload_avg = 0;

double sending_data[DATA_COUNT] = {0};
int sending_index = 0;
double sending_avg = 0;

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

    return 0;
}

void timer_log(char *key, double value)
{
    // DEBUG("Time elpased for %s is %f seconds\n", key, value);

    int i;

    if (strcmp(key, "GATHERING") == 0)
    {
        gathering_data[gathering_index++] = value;

        if (gathering_index >= DATA_COUNT) {
            // Calc avg
            double sum = 0;
            for (i = 0; i < DATA_COUNT; i++) {
                sum += gathering_data[i];
            }

            gathering_avg = sum / DATA_COUNT;
            // DEBUG("Avg for %s is %f seconds\n", key, gathering_avg);

            // Reset index
            gathering_index = 0;
        }
    }
    else if (strcmp(key, "PROCESSING") == 0)
    {
        processing_data[processing_index++] = value;

        if (processing_index >= DATA_COUNT) {
            // Calc avg
            double sum = 0;
            for (i = 0; i < DATA_COUNT; i++) {
                sum += processing_data[i];
            }

            processing_avg = sum / DATA_COUNT;
            // DEBUG("Avg for %s is %f seconds\n", key, processing_avg);

            // Reset index
            processing_index = 0;
        }
    }
    else if (strcmp(key, "PAYLOAD") == 0)
    {
        payload_data[payload_index++] = value;

        if (payload_index >= DATA_COUNT) {
            // Calc avg
            double sum = 0;
            for (i = 0; i < DATA_COUNT; i++) {
                sum += payload_data[i];
            }

            payload_avg = sum / DATA_COUNT;
            // DEBUG("Avg for %s is %f seconds\n", key, payload_avg);

            // Reset index
            payload_index = 0;
        }
    }
    else if (strcmp(key, "SENDING_WS") == 0)
    {
        sending_data[sending_index++] = value;

        if (sending_index >= DATA_COUNT) {
            // Calc avg
            double sum = 0;
            for (i = 0; i < DATA_COUNT; i++) {
                sum += sending_data[i];
            }

            sending_avg = sum / DATA_COUNT;
            // DEBUG("Avg for %s is %f seconds\n", key, sending_avg);

            // Reset index
            sending_index = 0;
        }
    }
}