#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <pthread.h>
#include <unistd.h>

#include "spectrum.h"
#include "dsp_common.h"
#include "../tools/helpers.h"

// Size
static unsigned int sensor_samples;

// FFT
static fftw_complex *input;
static fftw_complex *output;
static fftw_plan plan_forward;

static pthread_mutex_t spectrum_mutex;

// Thread
static pthread_t worker_thread;
static int running = 0;

static int new_data_available = 0;

static unsigned int buffer_size_squared = 0;

static double *power_spectrum;
static int new_spectrum_available = 0;

#define N 100

static void specrum_process()
{
    static unsigned int i = 0;
    static unsigned int j = 0;
    static double value = 0;

    if (new_data_available)
    {
        pthread_mutex_lock(&spectrum_mutex);

        fftw_execute(plan_forward);

        j = sensor_samples / 2;
        for (i = 0; i < sensor_samples; i++)
        {
            value = 10 * log10((creal(output[i]) * creal(output[i]) + cimag(output[i]) * cimag(output[i])) / buffer_size_squared);

            // https://stackoverflow.com/questions/12636613/how-to-calculate-moving-average-without-keeping-the-count-and-data-totalç
            power_spectrum[j] -= power_spectrum[j] / N;
            power_spectrum[j] += value / N;

            // Shift
            if (j > sensor_samples)
            {
                j = 0;
            }
            else
            {
                j++;
            }
        }

        new_spectrum_available = 1;
        new_data_available = 0;

        pthread_mutex_unlock(&spectrum_mutex);
    }
}

static void *spectrum_worker(void *user)
{
    UNUSED(user);

    while (running)
    {
        specrum_process();
        usleep(100);
    }

    DEBUG("Done\n");

    return 0;
}

void spectrum_init(unsigned int sensor_count)
{
    sensor_samples = sensor_count;
    power_spectrum = calloc(sensor_samples, sizeof(double));

    // FFT
    input = fftw_malloc(sizeof(fftw_complex) * sensor_samples);
    output = fftw_malloc(sizeof(fftw_complex) * sensor_samples);
    plan_forward = fftw_plan_dft_1d(sensor_samples, input, output, FFTW_FORWARD, FFTW_ESTIMATE);

    //
    buffer_size_squared = sensor_samples * sensor_samples;

    pthread_mutex_init(&spectrum_mutex, NULL);

    running = 1;
    pthread_create(&worker_thread, NULL, spectrum_worker, NULL);
}

void spectrum_data(const cmplx_dbl *signal, unsigned int len)
{
    if (new_data_available == 1)
    {
        return;
    }

    pthread_mutex_lock(&spectrum_mutex);

    memcpy(input, signal, len * sizeof(cmplx_dbl));
    new_data_available = 1;

    pthread_mutex_unlock(&spectrum_mutex);
}

int spectrum_available()
{
    return new_spectrum_available;
}

int spectrum_payload(char *buf, unsigned int buf_len)
{
    unsigned int count = 0;

    for (count = 0; count < MIN(buf_len, sensor_samples); count++)
    {
        double val = power_spectrum[count];
        if (val > 120)
        {
            val = 120;
        }
        if (val < -120)
        {
            val = -120;
        }
        buf[count] = (int8_t)val;
    }

    new_spectrum_available = 0;

    return count;
}

void spectrum_close()
{
    running = 0;
    pthread_join(worker_thread, NULL);

    free(power_spectrum);
    fftw_destroy_plan(plan_forward);
    fftw_free(input);
    fftw_free(output);
    pthread_mutex_destroy(&spectrum_mutex);
}