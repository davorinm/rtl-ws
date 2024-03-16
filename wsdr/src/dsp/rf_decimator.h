#ifndef RF_DECIMATOR_H
#define RF_DECIMATOR_H

#include "dsp_common.h"

typedef void (*rf_decimator_callback)(const cmplx_dbl *, int);

void rf_decimator_init();

int rf_decimator_set_parameters(int sample_rate, int buffer_size, int down_factor);

int rf_decimator_decimate(const cmplx_dbl *complex_signal, int len);

void rf_decimator_free();

#endif
