#include <stdlib.h>

#include "dsp.h"
#include "spectrum.h"
#include "filter.h"
#include "audio.h"
#include "decimator.h"
#include "../tools/helpers.h"
#include "../sensor/sensor.h"

fftw_complex *samples_input;
fftw_complex *samples_filter_output;

void dsp_init()
{
    // Get samples count from sensor
   unsigned int samples_count = sensor_get_buffer_size();

    // Get samples count from sensor
   unsigned int centerFrequency = sensor_get_freq();

    // Init samples
    samples_input = fftw_malloc(sizeof(fftw_complex) * samples_count);
    samples_filter_output = fftw_malloc(sizeof(fftw_complex) * samples_count);

    INFO("Initializing spectrum\n");
    spectrum_init(samples_count, samples_input);

    INFO("Initializing filter\n");
    filter_init(samples_count, centerFrequency, bandwidth, samplingRate);

    INFO("Initializing decimator\n");
    decimator_init();

    INFO("Initializing audio\n");
    audio_init();
}

void dsp_close()
{
    fftw_free(samples_input);
    fftw_free(samples_filter_output);

    INFO("Closing spectrum\n");
    spectrum_close();

    INFO("Closing filter\n");
    filter_close();

    INFO("Closing decimator\n");
    decimator_close();

    INFO("Closing audio\n");
    audio_close();
}

fftw_complex *dsp_samples()
{
    return samples_input;
}

void dsp_process()
{
    // spectrum
    spectrum_process(samples_input);

    // filter
    filter_process(samples_input, samples_filter_output);

    // decimate
    decimator_process(samples_filter_output);

    // demodulate
    audio_process(samples_filter_output);
}

int dsp_spectrum_available()
{
    return spectrum_available();
}

int dsp_spectrum_get_payload(char *buf, int buf_len)
{
    return spectrum_payload(buf, buf_len);
}

void dsp_audio_start()
{
}

void dsp_audio_stop()
{
}

int dsp_audio_available()
{
    return audio_available();
}

int dsp_audio_payload(char *buf, int buf_len)
{
    return audio_payload(buf, buf_len);
}