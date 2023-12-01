#include <string.h>
#include "resample.h"

static float half_band_kernel[] = {0.01824, 0.0, -0.11614, 0.0, 0.34790, 0.5, 0.34790, 0.0, -0.11614, 0.0, 0.01824};

void halfband_decimate(const float *input, float *output, int output_len, float *delay)
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
