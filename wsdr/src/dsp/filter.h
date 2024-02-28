#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>
#include <fftw3.h>

void filter_init(int samples_count, float centerFrequency, float bandwidth, float samplingRate);

void filter_close();

void filter_process(fftw_complex *input, fftw_complex *output);

#endif
