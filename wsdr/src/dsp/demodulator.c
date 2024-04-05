#include <stdlib.h>
#include <string.h>

#include "demodulator.h"
#include "../tools/helpers.h"

float atan2_approx(float imag, float real)
{
    static const float pi_by_2 = (float)(M_PI / 2);
    register float atan = 0;
    register float z = 0;

    if (real == 0)
    {
        if (imag > 0.0f)
            return pi_by_2;

        if (imag == 0)
            return 0;

        return -pi_by_2;
    }
    z = imag / real;
    if (fabs(z) < 1.0f)
    {
        atan = z / (1.0f + 0.28f * z * z);
        if (real < 0)
        {
            if (imag < 0.0f)
                return atan - M_PI;

            return atan + M_PI;
        }
    }
    else
    {
        atan = pi_by_2 - z / (z * z + 0.28f);
        if (imag < 0.0f)
            return atan - M_PI;
    }
    return atan;
}

void fm_demodulator(const cmplx_dbl *signal, float *demod_buffer, int len, float scale)
{
    static float prev_sample = 0;
    static float temp = 0;
    static int i = 0;

    for (i = 0; i < len; i++)
    {
        demod_buffer[i] = atan2_approx(imag_cmplx_s32(signal[i]), real_cmplx_s32(signal[i]));

        temp = demod_buffer[i];
        demod_buffer[i] -= prev_sample;
        prev_sample = temp;

        // hard limit output
        if (demod_buffer[i] > scale)
        {
            demod_buffer[i] = 1;
        }
        else if (demod_buffer[i] < -scale)
        {
            demod_buffer[i] = -1;
        }
        else
        {
            demod_buffer[i] /= scale;
        }
    }
}

static double polar_discriminant(const cmplx_dbl *a, const cmplx_dbl *b)
{
    cmplx_dbl c = *a * *b;
    double angle = atan2(imag_cmplx_s32(c), real_cmplx_s32(c));
    return angle / 3.14159 * (1 << 14);
}

void fm_demod(cmplx_dbl *lp, int lp_len, float *r, int *r_len, cmplx_dbl *pre)
{
    int i;
    double pcm;
    pcm = 
    r[0] = polar_discriminant(lp, pre);

    for (i = 1; i < (lp_len - 1); i++)
    {
        pcm = polar_discriminant(lp + i, lp + i - 1);
        r[i] = (int16_t)pcm;
    }
    
    pre = lp + lp_len - 1;
    *r_len = lp_len;
}

void am_demod(const cmplx_dbl *lp, int lp_len, double *r, int *r_len, double output_scale)
// todo, fix this extreme laziness
{
    int i;
    double pcm;
    for (i = 0; i < lp_len; i++)
    {
        // hypot uses floats but won't overflow
        // r[i/2] = (int16_t)hypot(lp[i], lp[i+1]);
        pcm = real_cmplx_s32(lp[i]) * real_cmplx_s32(lp[i]);
        pcm += imag_cmplx_s32(lp[i]) * imag_cmplx_s32(lp[i]);
        r[i] = sqrt(pcm) * output_scale;
    }
    *r_len = lp_len;
    // lowpass? (3khz)  highpass?  (dc)
}

void usb_demod(const cmplx_dbl *lp, int lp_len, double *r, int *r_len, double output_scale)
{
    int i;
    double pcm;
    for (i = 0; i < lp_len; i++)
    {
        pcm = real_cmplx_s32(lp[i]) + imag_cmplx_s32(lp[i]);
        r[i] = pcm * output_scale;
    }
    *r_len = lp_len;
}

void lsb_demod(const cmplx_dbl *lp, int lp_len, double *r, int *r_len, double output_scale)
{
    int i;
    double pcm;
    for (i = 0; i < lp_len; i++)
    {
        pcm = real_cmplx_s32(lp[i]) - imag_cmplx_s32(lp[i]);
        r[i] = pcm * output_scale;
    }
    *r_len = lp_len;
}
