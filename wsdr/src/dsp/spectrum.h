#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "dsp_common.h"

typedef struct spectrum spectrum;

spectrum* spectrum_alloc(int N);

int spectrum_add_cmplx_u8(spectrum* s, const cmplx_u8* src, double* power_spectrum, int len);

int spectrum_add_cmplx_s32(spectrum* s, const cmplx_s32* src, double* power_spectrum, int len);

void spectrum_free(spectrum* s);

#endif
