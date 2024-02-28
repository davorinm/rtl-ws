#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include "../tools/helpers.h"

#define N 11 // Filter order (adjust as needed)
static float filterCcoefficients[N];

static int length = 0; // Length of samples

void filter_init(int samples_count)
{
    length = samples_count;

    float centerFrequency = 1000000.0; // Center frequency in Hz
    float bandwidth = 100000.0;     // Bandwidth in Hz
    float samplingRate = 10000000.0;   // Sampling rate in Hz

    // Calculate normalized center frequency and bandwidth
    float normalizedCenter = 2.0 * M_PI * centerFrequency / samplingRate;
    float normalizedBandwidth = 2.0 * M_PI * bandwidth / samplingRate;

    INFO("filter_init normalizedCenter %f, normalizedBandwidth %f\n", normalizedCenter, normalizedBandwidth);

    // Design a bandpass FIR filter using a Hamming
    for (int i = 0; i < N; i++)
    {
        filterCcoefficients[i] = (sin(normalizedBandwidth / 2.0 * (i - (N - 1) / 2.0)) / (M_PI * (i - (N - 1) / 2.0))) * (0.54 - 0.46 * cos(2.0 * M_PI * i / (N - 1)));
    }
}

void filter_close()
{
}

void filter_process(fftw_complex *input, fftw_complex *output)
{
    // Apply the bandpass FIR filter
    for (int i = N; i < length; i++)
    {
        output[i] = 0 + 0 * I;

        // output[i].real = 0;
        // output[i].imag = 0;
        for (int j = 0; j < N; j++)
        {
            output[i] += filterCcoefficients[j] * input[i - j];
            // output[i] += (coefficients[j] * creal(input[i - j])) + (coefficients[j] * cimag(input[i - j]) * I);

            // output[i].real += coefficients[j] * input[i - j].real;
            // output[i].imag += coefficients[j] * input[i - j].imag;
        }
    }
}