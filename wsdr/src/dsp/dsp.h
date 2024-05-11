#ifndef DSP_H
#define DSP_H

#include <stdint.h>

void dsp_init();

uint64_t dsp_sensor_get_freq();
int dsp_sensor_set_frequency(uint64_t f);
uint32_t dsp_sensor_get_sample_rate();
int dsp_sensor_set_sample_rate(uint32_t fs);
uint32_t dsp_sensor_get_rf_band_width();
int dsp_sensor_set_rf_band_width(uint32_t bw);
uint32_t dsp_sensor_get_gain_mode();
int dsp_sensor_set_gain_mode(uint32_t gain_mode);
double dsp_sensor_get_gain();
int dsp_sensor_set_gain(double gain);
uint32_t dsp_sensor_get_buffer_size();
void dsp_sensor_start();
void dsp_sensor_stop();

void dsp_spectrum_start();
void dsp_spectrum_stop();
int dsp_spectrum_available();
int dsp_spectrum_payload(char *buf, int buf_len);

void dsp_filter_freq(int filter_freq);
void dsp_filter_width(int filter_freq_width);
int dsp_filter_get_freq();
int dsp_filter_get_width();

void dsp_audio_start();
void dsp_audio_stop();
int dsp_audio_available();
int dsp_audio_payload(char *buf, int buf_len);

void dsp_close();

#endif