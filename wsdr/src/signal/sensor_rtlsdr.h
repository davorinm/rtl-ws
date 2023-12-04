#ifndef RTL_SENSOR_H
#define RTL_SENSOR_H

#include <stdint.h>

int rtl_init();

int rtl_set_frequency(uint32_t f);

int rtl_set_sample_rate(uint32_t fs);

int rtl_set_gain(double gain);

uint32_t rtl_freq();

uint32_t rtl_sample_rate();

double rtl_gain();

int rtl_read_async(void (*callback)(unsigned char *, uint32_t, void *), void *user);

void rtl_cancel();

void rtl_close();

#endif
