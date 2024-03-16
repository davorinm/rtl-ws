#include <string.h>
#include <complex.h>

#include "rf_cic_decimate.h"
#include "dsp_common.h"

void cic_decimate(int R, const cmplx_dbl *src, int src_len, cmplx_dbl *dst, int dst_len, struct cic_delay_line *delay, int *processed_input, int *processed_output)
{
    register int src_idx = 0;
    register int dst_idx = 0;
    register cmplx_dbl integrator_curr_in = {0};
    register cmplx_dbl integrator_curr_out = {0};
    register cmplx_dbl integrator_prev_out = {0};
    register cmplx_dbl comb_prev_in = {0};

    integrator_prev_out = delay->integrator_prev_out;
    comb_prev_in = delay->comb_prev_in;

    if (dst_len * R != src_len)
    {
        return;
    }

    for (src_idx = 0; src_idx < src_len; src_idx++)
    {
        /* integrator y(n) = y(n-1) + x(n) */
        integrator_curr_in = src[src_idx];
        add_cmplx_s32(integrator_curr_in, integrator_prev_out, integrator_curr_out);

        /* resample */
        if (((src_idx + 1) % R) == 0)
        {
            /* comb y(n) = x(n) - x(n-1) */
            if (dst_idx >= dst_len)
            {
                return;
            }
            sub_cmplx_s32(integrator_curr_out, comb_prev_in, dst[dst_idx]);
            comb_prev_in = integrator_curr_out;

            *processed_input = src_idx + 1;
            *processed_output = dst_idx + 1;

            dst_idx++;
        }
        integrator_prev_out = integrator_curr_out;
    }

    delay->integrator_prev_out = integrator_prev_out;
    delay->comb_prev_in = comb_prev_in;
}
