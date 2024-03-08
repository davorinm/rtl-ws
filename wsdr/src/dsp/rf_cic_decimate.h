#ifndef CIC_DECIMATE_H
#define CIC_DECIMATE_H

#include "dsp_common.h"

struct cic_delay_line
{
    cmplx_dbl integrator_prev_out;
    cmplx_dbl comb_prev_in;
};

int cic_decimate(int R, const cmplx_dbl *src, int src_len, cmplx_dbl *dst, int dst_len, struct cic_delay_line *delay);

#endif