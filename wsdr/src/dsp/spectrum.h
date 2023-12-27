#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include <fftw3.h>

void spectrum_init();

fftw_complex *spectrum_data_input();

void spectrum_process();

int spectrum_available();

int spectrum_payload(char *buf, int buf_len);

void spectrum_close();

#endif
