#ifndef DSP_COMMON_H
#define DSP_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <complex.h>


typedef double complex cmplx_dbl;

#define add_cmplx_s32(a, b, result) result = a + b;

#define sub_cmplx_s32(a, b, result) result = a - b;

#define real_cmplx_s32(c) creal(c)

#define imag_cmplx_s32(c) cimag(c)

static inline float atan2_approx(float y, float x)
{
    static const float pi_by_2 = (float)(M_PI / 2);
    register float atan = 0;
    register float z = 0;

    if (x == 0)
    {
        if (y > 0.0f)
            return pi_by_2;

        if (y == 0)
            return 0;

        return -pi_by_2;
    }
    z = y / x;
    if (fabs(z) < 1.0f)
    {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0)
        {
            if (y < 0.0f)
                return atan - M_PI;

            return atan + M_PI;
        }
    }
    else
    {
        atan = pi_by_2 - z / (z * z + 0.28f);
        if (y < 0.0f)
            return atan - M_PI;
    }
    return atan;
}

#endif
