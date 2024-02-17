#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include "../sensor/sensor.h"
#include "../settings.h"
#include "filter.h"

// Size
unsigned int sensor_samples;

// FFT
double *window;
fftw_complex *input;
fftw_complex *output;
fftw_plan plan_forward;

double *power_spectrum;
int new_spectrum_available = 0;


fftw_complex *filtered_output;

static double win_hanning(int j, int n)
{
    double a = 2.0 * M_PI / (n - 1), w;
    w = 0.5 * (1.0 - cos(a * j));
    return (w);
}

void spectrum_init()
{
    sensor_samples = sensor_get_buffer_size();

    power_spectrum = calloc(sensor_samples, sizeof(double));

    // FFT
    window = fftw_malloc(sizeof(double) * sensor_samples);
    input = fftw_malloc(sizeof(fftw_complex) * sensor_samples);
    output = fftw_malloc(sizeof(fftw_complex) * (sensor_samples + 1));
    plan_forward = fftw_plan_dft_1d(sensor_samples, input, output, FFTW_FORWARD, FFTW_ESTIMATE);

    // Create window
    for (unsigned int i = 0; i < sensor_samples; i++)
    {
        window[i] = win_hanning(i, sensor_samples);
    }

    // Filter

    filtered_output = fftw_malloc(sizeof(fftw_complex) * sensor_samples);

    float centerFrequency = 1000000.0; // Center frequency in Hz
    float bandwidth = 100000.0;     // Bandwidth in Hz
    float samplingRate = 10000000.0;   // Sampling rate in Hz

    prepareBandPassFilter(centerFrequency, bandwidth, samplingRate);
}

void spectrum_close()
{
    fftw_free(window);
    fftw_free(input);
    fftw_free(output);
    fftw_destroy_plan(plan_forward);
}

fftw_complex *spectrum_data_input()
{
    return input;
}

void downConvert(fftw_complex *input, fftw_complex *output, int length, double centerFrequency, double samplingRate) {
    double phaseIncrement = 2.0 * M_PI * centerFrequency / samplingRate;
    double phase;
    double cosVal;
    double sinVal;

    for (int i = 0; i < length; i++) {
        phase = i * phaseIncrement;
        cosVal = cos(phase);
        sinVal = sin(phase);

        // Multiply with complex exponential function for down-conversion
        output[i].real = input[i].real * cosVal + input[i].imag * sinVal;
        output[i].imag = input[i].imag * cosVal - input[i].real * sinVal;
    }
}

void spectrum_process()
{
    unsigned int i = 0;
    unsigned int buffer_size_squared = sensor_samples * sensor_samples;
    
    // FFT

    fftw_execute(plan_forward);

    for (i = 0; i < sensor_samples; i++)
    {
        power_spectrum[i] = 10 * log10((creal(output[i]) * creal(output[i]) + cimag(output[i]) * cimag(output[i])) / buffer_size_squared);
    }

    new_spectrum_available = 1;

    // Fiter
    bandPassFilter(input, filtered_output, sensor_samples);

    // Down-convert the filtered signal to baseband
    downConvert(filteredOutput, basebandOutput, length, centerFrequency, samplingRate);

    decimate();

    demodulate();
}

int spectrum_available()
{
    return new_spectrum_available;
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))


int spectrum_payload(char *buf, unsigned int buf_len)
{
    unsigned int count = 0;

    for (count = 0; count < MIN(buf_len, sensor_samples); count++)
    // for (count = 0; count < MIN(buf_len, sensor_samples/2); count++)
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