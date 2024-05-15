#include <sys/time.h>
#include <string.h>

#include "stats.h"
#include "dict.h"
#include "helpers.h"

#define BILLION 1000000000.0
#define AVG 10
#define BUFFER_SIZE 1024

static dict_t *time_dict;
static char *data_buffer = NULL;

void stats_init()
{
    time_dict = dict_new(20);
    data_buffer = calloc(BUFFER_SIZE, 1);
}

void stats_close()
{
    dict_free(time_dict);
}

void stats_timer_start(struct timespec *begin)
{
    clock_gettime(CLOCK_REALTIME, begin);
}

void stats_timer_end(char *key, struct timespec *begin)
{
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);

    // time_spent = end - start
    double value = (end.tv_sec - begin->tv_sec) + ((end.tv_nsec - begin->tv_nsec) / BILLION);
    double avg_value = dict_value(key);

    avg_value -= avg_value / AVG;
    avg_value += value / AVG;

    dict_add(time_dict, key, avg_value);
}

char *stats_json()
{
    int pos = 0;
    int count = dict_count(time_dict);
    for (int i = 0; i < count; ++i)
    {
        char *key = dict_key(time_dict, i);
        double value = dict_value(time_dict, i);
        if (i == 0)
        {
            pos = sprintf(&data_buffer[pos], "{\"%s\":%lf,", key, value);
        }
        else if (i == count - 1)
        {
            pos = sprintf(&data_buffer[pos], "\"%s\":%lf}", key, value);
        }
        else
        {
            pos = sprintf(&data_buffer[pos], "\"%s\":%lf,", key, value);
        }
    }

    return data_buffer;
}