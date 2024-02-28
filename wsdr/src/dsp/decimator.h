#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <stdint.h>
#include <fftw3.h>

void decimator_init();

void decimator_close();

void decimator_process(fftw_complex *samples);

#endif