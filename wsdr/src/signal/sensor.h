#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H

#include "../dsp/dsp_common.h"

typedef void (*signal_source_callback)(const cmplx_u8 *, int);

void sensor_prepare();

int sensor_init();

uint32_t sensor_get_freq();

int sensor_set_frequency(uint32_t f);

uint32_t sensor_get_sample_rate();

int sensor_set_sample_rate(uint32_t fs);

int sensor_set_gain(double gain);

double sensor_get_gain();

void signal_source_add_callback(signal_source_callback callback);

void signal_source_remove_callbacks();

void sensor_start();

void sensor_stop();

void sensor_close();

#endif
