#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <pthread.h>

#include "spectrum.h"
#include "../tools/helpers.h"

#define SPECTRUM_EST_MS 250
#define FFT_POINTS 2048
#define FFT_AVERAGE 6

static int N;
static fftw_complex *in;
static fftw_complex *out;
static fftw_plan plan;

static pthread_mutex_t spectrum_mutex;
static double power_spectrum_transfer[FFT_POINTS] = {0};
static int spectrum_averaging_count = 0;
static uint64_t last_spectrum_estimation = 0;
static volatile int new_spectrum_available = 0;

static inline void calculate_power_spectrum(double *power_spectrum, int len)
{
    int i = 0;
    int idx = 0;
    int offset = N / 2;

    fftw_execute(plan);

    for (i = 0; i < len; i++)
    {
        idx = (offset + i) % len;
        if (idx > 0)
        {
            power_spectrum[i] += (out[idx][0] * out[idx][0] + out[idx][1] * out[idx][1]);
        }
        else
        {
            power_spectrum[i] += power_spectrum[i - 1];
        }
    }
}

static int spectrum_add_cmplx_s32(const cmplx_s32 *src, double *power_spectrum, int len)
{
    int i = 0;

    if (len != N) {
        return -1;
    }

    for (i = 0; i < N; i++)
    {
        in[i][0] = ((double)real_cmplx_s32(src[i])) / 128;
        in[i][1] = ((double)imag_cmplx_s32(src[i])) / 128;
    }

    calculate_power_spectrum(power_spectrum, len);

    return 0;
}

void spectrum_init()
{
    N = FFT_POINTS;
    in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    pthread_mutex_init(&spectrum_mutex, NULL);
}

void spectrum_process(const cmplx_s32 *signal, int len)
{
    static double power_spectrum[FFT_POINTS];
    int i = 0;
    int blocks = len / FFT_POINTS;

    if (timestamp() < (last_spectrum_estimation + SPECTRUM_EST_MS))
        return;

    blocks = blocks <= FFT_AVERAGE ? blocks : FFT_AVERAGE;
    memset(power_spectrum, 0, FFT_POINTS * sizeof(double));

    for (i = 0; i < blocks; i++)
    {
        if (spectrum_add_cmplx_s32(&(signal[i * FFT_POINTS]), power_spectrum, FFT_POINTS))
        {
            ERROR("Error while estimating spectrum.\n");
            return;
        }
    }

    last_spectrum_estimation = timestamp();
    new_spectrum_available = 1;

    pthread_mutex_lock(&spectrum_mutex);

    memcpy(power_spectrum_transfer, power_spectrum, FFT_POINTS * sizeof(double));
    spectrum_averaging_count = blocks;

    pthread_mutex_unlock(&spectrum_mutex);
}

int spectrum_available()
{
    return new_spectrum_available;
}

int spectrum_payload(char *buf, int buf_len)
{
    static double power_spectrum_local[FFT_POINTS];
    int spectrum_averaging_count_local = 0;
    int idx = 0;
    int len = 0;

    pthread_mutex_lock(&spectrum_mutex);

    memcpy(power_spectrum_local, power_spectrum_transfer, FFT_POINTS * sizeof(double));
    spectrum_averaging_count_local = spectrum_averaging_count;

    pthread_mutex_unlock(&spectrum_mutex);

    if (spectrum_averaging_count > 0)
    {
        for (idx = 0; idx < FFT_POINTS; idx++)
        {
            int m = 10 * log10(fabs(power_spectrum_local[idx] / spectrum_averaging_count_local));
            m = m >= 0 ? m : 0;
            m = m <= 255 ? m : 255;
            buf[len++] = (char)m;
        }
    }

    new_spectrum_available = 0;

    return len;
}

void spectrum_close()
{
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    pthread_mutex_destroy(&spectrum_mutex);
}
