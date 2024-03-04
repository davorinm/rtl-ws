#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include <fftw3.h>

void spectrum_init(unsigned int sensor_count, fftw_complex *input);

void spectrum_close();

void spectrum_process();

int spectrum_available();

int spectrum_payload(char *buf, int buf_len);

#endif