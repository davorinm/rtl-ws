#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "settings.h"

// FFT
double *window;
fftw_complex *input;
fftw_complex *output;
fftw_plan plan_forward;

// Buffer
unsigned int buffer_size = PLUTO_SAMPLES_PER_READ;

static double power_spectrum[PLUTO_SAMPLES_PER_READ];
int new_spectrum_available = 0;

static double win_hanning(int j, int n)
{
    double a = 2.0 * M_PI / (n - 1), w;
    w = 0.5 * (1.0 - cos(a * j));
    return (w);
}

void spectrum_init()
{

    // FFT
    window = fftw_malloc(sizeof(double) * buffer_size);
    input = fftw_malloc(sizeof(fftw_complex) * buffer_size);
    output = fftw_malloc(sizeof(fftw_complex) * (buffer_size + 1));
    plan_forward = fftw_plan_dft_1d(buffer_size, input, output, FFTW_FORWARD, FFTW_ESTIMATE);

    // Create window
    for (unsigned int i = 0; i < buffer_size; i++)
    {
        window[i] = win_hanning(i, buffer_size);
    }
}

fftw_complex *spectrum_data_input()
{
    return input;
}

void spectrum_process()
{
    fftw_execute(plan_forward);

    int i;

    for (i = 0; i < buffer_size; i++)
    {
        power_spectrum[i] = sqrt(creal(output[i]) * creal(output[i]) + cimag(output[i]) * cimag(output[i]));
    }

    new_spectrum_available = 1;
}

int spectrum_available()
{
    return new_spectrum_available;
}

int spectrum_payload(char *buf, int buf_len, double sw_spectrum_gain_db)
{
    int spectrum_averaging_count = 0;
    int idx = 0;
    int len = 0;
    //double linear_energy_gain = pow(10, sw_spectrum_gain_db / 10);

    for (idx = 0; idx < buffer_size; idx++)
    {
        if (idx >= buf_len) {
            break;
        }

        int m = 10 * log10(fabs(power_spectrum[idx]));
        m = m >= 0 ? m : 0;
        m = m <= 255 ? m : 255;
        buf[len++] = (char)m;
    }

    new_spectrum_available = 0;

    return len;
}

void spectrum_close()
{
    fftw_free(window);
    fftw_free(input);
    fftw_free(output);
    fftw_destroy_plan(plan_forward);
}