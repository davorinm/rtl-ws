#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>
#include <fftw3.h>

void filter_init();

void filter_close();

void filter_process(fftw_complex *samples);

#endif
