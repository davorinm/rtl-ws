#ifndef CIC_DECIMATE_H
#define CIC_DECIMATE_H

#include "dsp_common.h"

#define HALF_BAND_N 11

struct cic_delay_line
{
    cmplx_s32 integrator_prev_out;
    cmplx_s32 comb_prev_in;
};

int cic_decimate(int R, const cmplx_u8 *src, int src_len, cmplx_s32 *dst, int dst_len, struct cic_delay_line *delay);

#endif