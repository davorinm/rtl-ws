#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H

#include <fftw3.h>

int sensor_init();

uint32_t sensor_get_freq();
int sensor_set_frequency(uint32_t f);

uint32_t sensor_get_sample_rate();
int sensor_set_sample_rate(uint32_t fs);

uint32_t sensor_get_rf_band_width();
int sensor_set_rf_band_width(uint32_t bw);

uint32_t sensor_get_gain_mode();
int sensor_set_gain_mode(uint32_t gain_mode);

double sensor_get_gain();
int sensor_set_gain(double gain);

uint32_t sensor_get_buffer_size();

void sensor_start();
void sensor_stop();
void sensor_close();

#endif
