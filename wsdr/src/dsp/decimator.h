#ifndef DECIMATOR_H
#define DECIMATOR_H

#include "dsp_common.h"

typedef void (*rf_decimator_callback)(const cmplx_s32 *, int);

typedef struct rf_decimator rf_decimator;

rf_decimator *decimator_init(rf_decimator_callback callback);

int rf_decimator_set_parameters(rf_decimator *d, int sample_rate, int down_factor);

int rf_decimator_decimate(rf_decimator *d, const cmplx_s32 *complex_signal, int len);

void rf_decimator_free(rf_decimator *d);

#endif
