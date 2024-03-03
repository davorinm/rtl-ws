#include <stdlib.h>

#include "dsp.h"
#include "spectrum.h"
#include "audio.h"
#include "decimator.h"
#include "../tools/helpers.h"
#include "../sensor/sensor.h"

#include <math.h>
#include <complex.h>
#include <fftw3.h>

fftw_complex *samples_input;
fftw_complex *samples_filter_output;
cmplx_s32 *samples_input32;

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
    samples_input32 = (cmplx_s32 *)realloc(samples_input32, samples_count * sizeof(cmplx_s32));

    INFO("Initializing spectrum\n");
    spectrum_init(samples_count, samples_input);

    INFO("Initializing decimator\n");
    decim = decimator_init(audio_process);
    rf_decimator_set_parameters(decim, samplingRate, samplingRate / DECIMATED_TARGET_BW_HZ);

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

static unsigned int i = 0;

void dsp_process()
{
    // spectrum
    spectrum_process(samples_input);

    for (i = 0; i < samples_count; i++) {

        // INFO("DSP R %f, %f\n", creal(samples_input[i]), 128 * creal(samples_input[i]));
        // INFO("DSP I %f, %f\n", cimag(samples_input[i]), 128 * cimag(samples_input[i]));


        samples_input32[i].p.re = 128 * creal(samples_input[i]) * 2;               
        samples_input32[i].p.im = 128 * cimag(samples_input[i]) * 2;
    }

    // decimator
    rf_decimator_decimate(decim, samples_input32, samples_count);
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
    audio_reset();
}

void dsp_audio_stop()
{
    audio_reset();
}

int dsp_audio_available()
{
    return audio_available();
}

int dsp_audio_payload(char *buf, int buf_len)
{
    return audio_payload(buf, buf_len);
}