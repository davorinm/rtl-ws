#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "../settings.h"

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
    unsigned int i = 0;
    unsigned int buffer_size_squared = buffer_size * buffer_size;
    
    fftw_execute(plan_forward);

    for (i = 0; i < buffer_size; i++)
    {
        power_spectrum[i] = 10 * log10((creal(output[i]) * creal(output[i]) + cimag(output[i]) * cimag(output[i])) / buffer_size_squared);
    }

    // for (i = 0; i < 200; i++)
    // {
    //     printf("POW %f \n", power_spectrum[i]);
    // }

    new_spectrum_available = 1;
}

int spectrum_available()
{
    return new_spectrum_available;
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))


int spectrum_payload(char *buf, unsigned int buf_len)
{
    unsigned int count = 0;

    for (count = 0; count < MIN(buf_len, buffer_size); count++)
    {   
        double val = power_spectrum[count];
        if (val > 120) {
            val = 120;
        }
        if (val < -120) {
            val = -120;
        }
        buf[count] = (int8_t)val;
    }

    new_spectrum_available = 0;

    return count;
}

void spectrum_close()
{
    fftw_free(window);
    fftw_free(input);
    fftw_free(output);
    fftw_destroy_plan(plan_forward);
}