#ifndef RESAMPLE_H
#define RESAMPLE_H

#define HALF_BAND_N 11

/* delay line has to have HALF_BAND_N - 1 elements */
void halfband_decimate(const float *input, float *output, int output_len, float *delay);

#endif
