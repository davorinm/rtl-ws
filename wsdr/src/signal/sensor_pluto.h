#ifndef SENSOR_PLUTO_H
#define SENSOR_PLUTO_H

#include <stdint.h>

struct pluto_dev;

int pluto_init();

int pluto_set_frequency(uint32_t f);

int pluto_set_sample_rate(uint32_t fs);

int pluto_set_gain(double gain);

uint32_t pluto_freq();

uint32_t pluto_sample_rate();

double pluto_gain();

int pluto_read_async(void (*callback)(unsigned char *, uint32_t, void *), void *user);

void pluto_cancel();

void pluto_close();

#endif