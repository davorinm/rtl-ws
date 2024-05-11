#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#include "dsp.h"
#include "spectrum.h"
#include "audio.h"
#include "rf_decimator.h"
#include "../tools/helpers.h"
#include "../sensor/sensor.h"
#include "../tools/timer.h"

#define DECIMATED_TARGET_BW_HZ 48000 * 2

static int audio_process_enabled = 0;
static int spectrum_process_enabled = 1;

static struct timespec time_measure;
static double time_spent;

static void demodulator_cb(const cmplx_dbl *signal, int len)
{
    timer_start(&time_measure);

    if (audio_process_enabled)
    {
        rf_decimator_decimate(signal, len);
    }

    timer_end(&time_measure, &time_spent);
    timer_log("AUDIO", time_spent);
}

static void spectrum_cb(const cmplx_dbl *signal, int len)
{
    timer_start(&time_measure);

    if (spectrum_process_enabled)
    {
        spectrum_process(signal, len);
    }

    timer_end(&time_measure, &time_spent);
    timer_log("SPECTRUM", time_spent);
}

static void signal_cb(const cmplx_dbl *signal, int len)
{
    demodulator_cb(signal, len);
    spectrum_cb(signal, len);
}

void dsp_init()
{
    INFO("Initializing sensor\n");
    sensor_init();

    INFO("Initializing audio processing...\n");
    audio_init();

    unsigned int sample_rate = sensor_get_sample_rate();
    unsigned int buffer_size = sensor_get_buffer_size();

    rf_decimator_init(audio_process);
    rf_decimator_set_parameters(sample_rate, buffer_size, DECIMATED_TARGET_BW_HZ);

    spectrum_init(buffer_size);

    signal_source_add_callback(signal_cb);
}

// Sensor

uint64_t dsp_sensor_get_freq()
{
    return sensor_get_freq();
}

int dsp_sensor_set_frequency(uint64_t f)
{
    return sensor_set_frequency(f);
}

uint32_t dsp_sensor_get_sample_rate()
{
    return sensor_get_sample_rate();
}

int dsp_sensor_set_sample_rate(uint32_t fs)
{
    return sensor_set_sample_rate(fs);
}

uint32_t dsp_sensor_get_rf_band_width()
{
    return sensor_get_rf_band_width();
}

int dsp_sensor_set_rf_band_width(uint32_t bw)
{
    return sensor_set_rf_band_width(bw);
}

uint32_t dsp_sensor_get_gain_mode()
{
    return sensor_get_gain_mode();
}

int dsp_sensor_set_gain_mode(uint32_t gain_mode)
{
    return sensor_set_gain_mode(gain_mode);
}

double dsp_sensor_get_gain()
{
    return sensor_get_gain();
}

int dsp_sensor_set_gain(double gain)
{
    return sensor_set_gain(gain);
}

uint32_t dsp_sensor_get_buffer_size()
{
    return sensor_get_buffer_size();
}

void dsp_sensor_start()
{
    sensor_start();
}

void dsp_sensor_stop()
{
    sensor_stop();
}

// Spectrum

void dsp_spectrum_start()
{
    spectrum_process_enabled = 1;
}

void dsp_spectrum_stop()
{
    spectrum_process_enabled = 0;
}

int dsp_spectrum_available()
{
    return spectrum_available();
}

int dsp_spectrum_payload(char *buf, int buf_len)
{
    return spectrum_payload(buf, buf_len);
}

// Filter

void dsp_filter_freq(int filter_freq)
{
}

void dsp_filter_width(int filter_freq_width)
{
}

int dsp_filter_get_freq()
{
    return 19000;
}

int dsp_filter_get_width()
{
    return 500;
}

// Audio

void dsp_audio_start()
{
    audio_process_enabled = 1;
    audio_start();
}

void dsp_audio_stop()
{
    audio_process_enabled = 0;
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

// Close

void dsp_close()
{
    INFO("Closing sensor\n");
    sensor_close();

    INFO("Remove callback\n");
    signal_source_remove_callback();

    INFO("rf_decimator_free\n");
    rf_decimator_free();

    INFO("spectrum_close\n");
    spectrum_close();

    INFO("Closing audio processing...\n");
    audio_close();
}
