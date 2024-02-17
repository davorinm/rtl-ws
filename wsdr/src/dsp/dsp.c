#include <stdlib.h>

#include "dsp.h"
#include "spectrum.h"
#include "filter.h"
#include "audio.h"
#include "../tools/helpers.h"

unsigned int samples_size;
fftw_complex *samples_input;
fftw_complex *samples_spectrum;

static void dsp_copy(fftw_complex *dst, fftw_complex *src, int size) {
    memcpy(dst, src, size);
}

void dsp_init()
{
    INFO("Initializing spectrum\n");
    spectrum_init();

    INFO("Initializing filter\n");
    filter_init();

    INFO("Initializing audio\n");
    audio_init();
}

void dsp_close()
{
    INFO("Closing spectrum\n");
    spectrum_close();

    INFO("Closing filter\n");
    filter_close();

    INFO("Closing audio\n");
    audio_close();
}

int dsp_process(fftw_complex *samples)
{
    // copy samples for spectrum and filter
    dsp_copy(samples_spectrum, samples_input, samples_size);

    // spectrum
    spectrum_process(samples_spectrum);

    // filter
    filter_process(samples_input);

    // decimate
    decimator_process(samples_input);

    // demodulate
    audio_process(samples_input);

    return 0;
}

int dsp_spectrum_available()
{
    return spectrum_available();
}

int dsp_spectrum_get_payload(char *buf, int buf_len)
{
    return spectrum_payload(buf, buf_len);
}

int dsp_audio_available()
{
    return audio_available();
}

int dsp_audio_payload(char *buf, int buf_len)
{
    return audio_payload(buf, buf_len);
}