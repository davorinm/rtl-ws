#ifndef CIC_DECIMATE_H
#define CIC_DECIMATE_H

#include "dsp_common.h"

#define HALF_BAND_N 11

typedef struct cic_delay_line
{
    cmplx_dbl integrator_prev_out;
    cmplx_dbl comb_prev_in;
} cic_delay_line;

/* delay line has to have HALF_BAND_N - 1 elements */
void rf_halfband_decimate(const float *input, float *output, int output_len, float *delay);

void cic_decimate(int R, const cmplx_dbl *src, int src_len, cmplx_dbl *dst, int dst_len, cic_delay_line *delay, int *processed_input, int *processed_output);

#endif