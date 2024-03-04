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

static rf_decimator *decim = NULL;

static void signal_cb(const cmplx_s32 *signal, int len)
{
    rf_decimator_decimate_cmplx_s32(decim, signal, len);
    spectrum_process(signal, len);
}

void dsp_init()
{
    INFO("Initializing audio processing...\n");
    audio_init();

    unsigned int sample_rate = sensor_get_sample_rate();

    decim = rf_decimator_alloc();
    rf_decimator_set_parameters(decim, sample_rate, sample_rate / DECIMATED_TARGET_BW_HZ);

    spectrum_init();

    signal_source_add_callback(signal_cb);

    rf_decimator_add_callback(decim, audio_fm_demodulator);
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
    signal_source_remove_callback();

    rf_decimator_free(decim);

    spectrum_close();

    INFO("Closing audio processing...\n");
    audio_close();
}
