#include <string.h>
#include <complex.h>

#include "rf_cic_decimate.h"
#include "dsp_common.h"

static float half_band_kernel[] = {0.01824, 0.0, -0.11614, 0.0, 0.34790, 0.5, 0.34790, 0.0, -0.11614, 0.0, 0.01824};

void rf_halfband_decimate(const float *input, float *output, int output_len, float *delay)
{
    register int n = 0;
    register int k = 0;
    register int idx = 0;

    for (n = 0; n < output_len; n++)
    {
        idx = 2 * n - HALF_BAND_N / 2;
        output[n] = half_band_kernel[HALF_BAND_N / 2] * (idx >= 0 ? input[idx] : delay[(HALF_BAND_N - 1) + idx]);

        /* all odd samples in half band kernel are zeroes except the middle one */
        for (k = 0; k < HALF_BAND_N; k += 2)
        {
            idx = 2 * n - k;
            output[n] += half_band_kernel[k] * (idx >= 0 ? input[idx] : delay[(HALF_BAND_N - 1) + idx]);
        }
    }
    memcpy(delay, &(input[2 * output_len - (HALF_BAND_N - 1)]), (HALF_BAND_N - 1) * sizeof(float));
}

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

    for (src_idx = 0; src_idx < src_len && dst_idx < dst_len; src_idx++)
    {
        /* integrator y(n) = y(n-1) + x(n) */
        integrator_curr_in = src[src_idx];
        add_cmplx_s32(integrator_curr_in, integrator_prev_out, integrator_curr_out);

        /* resample */
        if (((src_idx + 1) % R) == 0)
        {
            /* comb y(n) = x(n) - x(n-1) */
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
