#ifndef STATS_H
#define STATS_H

#include <time.h>
#include <sys/time.h>

void stats_init();

void stats_close();

void stats_timer_start(struct timespec *begin);

void stats_timer_end(char *key, struct timespec *begin);

char *stats_json();

#endif