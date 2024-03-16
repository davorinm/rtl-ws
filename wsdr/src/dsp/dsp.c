#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#include "dsp.h"
#include "rf_decimator.h"
#include "spectrum.h"
#include "audio.h"
#include "../tools/helpers.h"
#include "../sensor/sensor.h"

#define DECIMATED_TARGET_BW_HZ 192000

static void signal_cb(const cmplx_dbl *signal, int len)
{
    rf_decimator_decimate(signal, len);
    spectrum_process(signal, len);
}

void dsp_init()
{
    INFO("Initializing audio processing...\n");
    audio_init();

    unsigned int sample_rate = sensor_get_sample_rate();
    unsigned int buffer_size = sensor_get_buffer_size();

    rf_decimator_alloc();
    rf_decimator_set_parameters(sample_rate, buffer_size, sample_rate / DECIMATED_TARGET_BW_HZ);

    spectrum_init(buffer_size);

    signal_source_add_callback(signal_cb);

    rf_decimator_add_callback(audio_fm_demodulator);
}

int dsp_spectrum_available()
{
    return spectrum_available();
}

int dsp_spectrum_payload(char *buf, int buf_len)
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
    return audio_new_audio_available();
}

int dsp_audio_payload(char *buf, int buf_len)
{
    return audio_get_audio_payload(buf, buf_len);
}

void dsp_close()
{
    INFO("Remove callback\n");
    signal_source_remove_callback();

    INFO("rf_decimator_free\n");
    rf_decimator_free();

    INFO("spectrum_close\n");
    spectrum_close();

    INFO("Closing audio processing...\n");
    audio_close();
}
