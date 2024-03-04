#ifndef RF_DECIMATOR_H
#define RF_DECIMATOR_H

#include "dsp_common.h"

typedef void (*rf_decimator_callback)(const cmplx_s32 *, int);

typedef struct rf_decimator rf_decimator;

rf_decimator *rf_decimator_alloc();

void rf_decimator_add_callback(rf_decimator *d, rf_decimator_callback callback);

int rf_decimator_set_parameters(rf_decimator *d, int sample_rate, int down_factor);

int rf_decimator_decimate_cmplx_s32(rf_decimator *d, const cmplx_s32 *complex_signal, int len);

void rf_decimator_remove_callbacks(rf_decimator *d);

void rf_decimator_free(rf_decimator *d);

#endif
