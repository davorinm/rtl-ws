#include <stdlib.h>

#include "dsp.h"
#include "spectrum.h"
#include "audio.h"
#include "decimator.h"
#include "../tools/helpers.h"
#include "../sensor/sensor.h"

fftw_complex *samples_input;
fftw_complex *samples_filter_output;

static rf_decimator *decim = NULL;

unsigned int samples_count = 0;

void dsp_init()
{
    // Get data from sensor
    samples_count = sensor_get_buffer_size();
    unsigned int centerFrequency = sensor_get_freq();
    unsigned int bandwidth = sensor_get_freq();
    unsigned int samplingRate = sensor_get_sample_rate();

    // Init samples
    samples_input = fftw_malloc(sizeof(fftw_complex) * samples_count);
    samples_filter_output = fftw_malloc(sizeof(fftw_complex) * samples_count);

    INFO("Initializing spectrum\n");
    spectrum_init(samples_count, samples_input);

    INFO("Initializing decimator\n");
    decim = rf_decimator_alloc();
    rf_decimator_set_parameters(decim, samplingRate, samplingRate / DECIMATED_TARGET_BW_HZ);
    rf_decimator_add_callback(decim, audio_process);

    INFO("Initializing audio\n");
    audio_init();
}

void dsp_close()
{
    fftw_free(samples_input);
    fftw_free(samples_filter_output);

    INFO("Closing spectrum\n");
    spectrum_close();

    INFO("Closing decimator\n");
    rf_decimator_free(decim);

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

    // decimator
    rf_decimator_decimate(decim, samples_input, samples_count);
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