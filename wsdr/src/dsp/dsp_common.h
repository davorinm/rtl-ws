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

#endif
