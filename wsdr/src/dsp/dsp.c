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
#include "../tools/timer.h"

#define DECIMATED_TARGET_BW_HZ 48000 * 2

static void signal_cb(const cmplx_dbl *signal, int len)
{    
static struct timespec time;
static double time_spent;

    timer_start(&time);
    
    rf_decimator_decimate(signal, len);
        
    timer_end(&time, &time_spent);
    timer_log("AUDIO", time_spent);
        
    timer_start(&time);

    spectrum_process(signal, len);
        
    timer_end(&time, &time_spent);
    timer_log("SPECTRUM", time_spent);
}

void dsp_init()
{
    INFO("Initializing audio processing...\n");
    audio_init();

    unsigned int sample_rate = sensor_get_sample_rate();
    unsigned int buffer_size = sensor_get_buffer_size();

    rf_decimator_init(audio_fm_demodulator);
    rf_decimator_set_parameters(sample_rate, buffer_size, DECIMATED_TARGET_BW_HZ);

    spectrum_init(buffer_size);

    signal_source_add_callback(signal_cb);
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
    audio_start();
}

void dsp_audio_stop()
{
    audio_stop();
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
