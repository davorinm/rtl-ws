#ifndef DEMODULATOR_H
#define DEMODULATOR_H

#include "dsp_common.h"

void fm_demodulator(const cmplx_dbl *signal, float *demod_buffer, int len, float scale);

#endif