#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#define N 51 // Filter order (adjust as needed)
float coefficients[N];

void filter_init() {

}

void filter_close() {

}

void filter_process(fftw_complex *samples) {
    
}

void prepareBandPassFilter(float centerFrequency, float bandwidth, float samplingRate)
{
    // Calculate normalized center frequency and bandwidth
    float normalizedCenter = 2.0 * M_PI * centerFrequency / samplingRate;
    float normalizedBandwidth = 2.0 * M_PI * bandwidth / samplingRate;

    // Design a bandpass FIR filter using a Hamming
    for (int i = 0; i < N; i++)
    {
        coefficients[i] = (sin(normalizedBandwidth / 2.0 * (i - (N - 1) / 2.0)) / (M_PI * (i - (N - 1) / 2.0))) * (0.54 - 0.46 * cos(2.0 * M_PI * i / (N - 1)));
    }
}

void bandPassFilter(fftw_complex *input, fftw_complex *output, int length)
{
    // Apply the bandpass FIR filter
    for (int i = N; i < length; i++)
    {
        output[i] = 0 + 0 * I;

        // output[i].real = 0;
        // output[i].imag = 0;
        for (int j = 0; j < N; j++)
        {
            output[i] += coefficients[j] * input[i - j];
            // output[i] += (coefficients[j] * creal(input[i - j])) + (coefficients[j] * cimag(input[i - j]) * I);

            // output[i].real += coefficients[j] * input[i - j].real;
            // output[i].imag += coefficients[j] * input[i - j].imag;
        }
    }
}

int mainttt()
{
    // Example usage
    int length = 100;
    Complex input[100]; // Assuming an input of 100 complex samples
    Complex output[100];
    float centerFrequency = 1000000.0; // Center frequency in Hz
    float bandwidth = 100000.0;     // Bandwidth in Hz
    float samplingRate = 10000000.0;   // Sampling rate in Hz

    // Populate your input array with complex samples

    prepareBandPassFilter(centerFrequency, bandwidth, samplingRate);
    bandPassFilter(input, output, length);

    // Print the filtered output
    for (int i = 0; i < length; i++)
    {
        printf("(%f + %fi) ", output[i].real, output[i].imag);
    }

    return 0;
}