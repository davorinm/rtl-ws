#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <rtl-sdr.h>

#include <math.h>

#include <stdbool.h>

#include "sensor.h"
#include "../tools/helpers.h"
#include "../tools/timer.h"
#include "../dsp/dsp.h"

typedef struct rtl_dev
{
    rtlsdr_dev_t *dev;
    uint32_t device_index;
    uint32_t f;
    uint32_t fs;
    double gain;
    uint32_t gain_mode;
    uint32_t samples;
} rtl_dev_t;

static rtl_dev_t *dev = NULL;

static uint8_t *buf = NULL;
static cmplx_s32 *samples_output;

// Thread
static pthread_t worker_thread;
static bool running = false;

// Callback
static signal_source_callback callback_function;
static pthread_mutex_t callback_mutex;

static void signal_source_callback_notifier()
{
    if (callback_function == NULL)
    {
        return;
    }

    pthread_mutex_lock(&callback_mutex);
    callback_function(samples_output, dev->samples);
    pthread_mutex_unlock(&callback_mutex);
}

static int sensor_loop()
{
    int samples_read;
    int retval;

    // Refill RX buffer
    retval = rtlsdr_read_sync(dev->dev, buf, dev->samples * 2, &samples_read);
    if (retval < 0)
    {
        DEBUG("Samples read failed: %d\n", retval);
        return 0;
    }

    // convert buffer from IQ to complex
    int i;
    int n = 0;
    for (i = 0; i < samples_read; i += 2)
    {
        // in_c[n] = ((buf[i] + I * buf[i + 1]) - 127) / 127; TODO: use double, divide with 127
        samples_output[n].p.re = (int32_t)buf[i] - 128;
        samples_output[n].p.im = (int32_t)buf[i + 1] - 128;



        // __real__ in_c[n] = (double)buf[i] / 127; // sample is 127 for zero signal,so 127 +/-127
        // __imag__ in_c[n] = (double)buf[i + 1] / 127;
        // printf("%f + %f\n", __real__  in[i], __imag__ in[i]);
        // Increment samples_output pointer
        n++;
    }

    return 0;
}

static void *worker(void *user)
{
    UNUSED(user);

    struct timespec time;
    double time_spent;

    int status = 0;
    DEBUG("Reading signal from sensor\n");

    while (running)
    {
        timer_start(&time);

        status = sensor_loop();
        if (status < 0)
        {
            ERROR("Read failed with status %d\n", status);
            continue;
        }

        timer_end(&time, &time_spent);
        timer_log("GATHERING", time_spent);

        timer_start(&time);

        signal_source_callback_notifier();

        timer_end(&time, &time_spent);
        timer_log("PROCESSING", time_spent);
    }

    DEBUG("Done reading signal from sensor\n");

    return 0;
}

int sensor_init()
{
    int r = 0;

    // Device
    dev = (rtl_dev_t *)calloc(1, sizeof(rtl_dev_t));
    dev->device_index = 0;
    dev->f = 102800000;
    dev->fs = 1920000;
    dev->gain_mode = 0;
    dev->gain = 25;
    dev->samples = 4096 * 2;

    // Buffers
    buf = calloc(1, dev->samples * sizeof(uint8_t) * 2);
    samples_output = calloc(1, dev->samples * sizeof(cmplx_s32));

    // Lock
    pthread_mutex_init(&callback_mutex, NULL);

    // Open
    DEBUG("rtlsdr_open...\n");
    r = rtlsdr_open(&dev->dev, dev->device_index);
    if (r < 0)
    {
        ERROR("Failed to open rtlsdr device #%d.\n", dev->device_index);
        return r;
    }

    DEBUG("rtl_set_sample_rate...\n");
    r = rtlsdr_set_sample_rate(dev->dev, dev->fs);
    if (r < 0)
    {
        ERROR("Failed to set sample rate \n");
        return r;
    }

    DEBUG("rtl_set_frequency...\n");
    r = rtlsdr_set_center_freq(dev->dev, dev->f);
    if (r < 0)
    {
        ERROR("Failed to set frequency \n");
        return r;
    }

    DEBUG("Setting rtl gain mode to manual...\n");
    r = rtlsdr_set_tuner_gain_mode(dev->dev, dev->gain_mode);
    if (r < 0)
    {
        ERROR("Failed to enable manual gain.\n");
        return r;
    }

    DEBUG("Setting rtl gain to %f...\n", dev->gain);
    r = rtlsdr_set_tuner_gain(dev->dev, (int)(dev->gain * 10));
    if (r < 0)
    {

        ERROR("Failed to set gain.\n");
        return r;
    }

    r = rtlsdr_reset_buffer(dev->dev);
    if (r < 0)
    {
        ERROR("Failed to reset buffers.\n");
        return r;
    }

    DEBUG("rtl_init returns %d\n", r);
    return r;
}

void signal_source_add_callback(signal_source_callback callback)
{
    pthread_mutex_lock(&callback_mutex);
    callback_function = callback;
    pthread_mutex_unlock(&callback_mutex);
}

void signal_source_remove_callback()
{
    pthread_mutex_lock(&callback_mutex);
    callback_function = NULL;
    pthread_mutex_unlock(&callback_mutex);
}

uint32_t sensor_get_freq()
{
    return dev->f;
}

int sensor_set_frequency(uint32_t f)
{
    int r = 0;
    if (dev->f != f)
    {
        r = rtlsdr_set_center_freq(dev->dev, f);
        if (r < 0)
        {
            ERROR("Failed to set center freq.\n");
        }
        else
        {
            INFO("Tuned to %u Hz.\n", f);
            dev->f = f;
        }
    }
    return r;
}

uint32_t sensor_get_rf_band_width()
{
    return dev->fs;
}

int sensor_set_rf_band_width(uint32_t bw)
{
    UNUSED(bw);
    return 0;
}

uint32_t sensor_get_sample_rate()
{
    return dev->fs;
}

int sensor_set_sample_rate(uint32_t fs)
{
    int r = 0;
    if (dev->fs != fs)
    {
        r = rtlsdr_set_sample_rate(dev->dev, fs);
        if (r < 0)
        {
            ERROR("Failed to set sample rate.\n");
        }
        else
        {
            INFO("Sample rate set to %u Hz.\n", fs);
            dev->fs = fs;
        }
    }
    return r;
}

uint32_t sensor_get_gain_mode()
{
    return dev->gain_mode;
}

int sensor_set_gain_mode(uint32_t gain_mode)
{
    int r = 0;
    if (dev->gain_mode != gain_mode)
    {
        r = rtlsdr_set_tuner_gain_mode(dev->dev, gain_mode);
        if (r < 0)
        {
            ERROR("Failed to set tuner gain mode.\n");
        }
        else
        {
            INFO("Tuner gain mode set to %i dB.\n", gain_mode);
            dev->gain_mode = gain_mode;
        }
    }
    return r;
}

double sensor_get_gain()
{
    return dev->gain;
}

int sensor_set_gain(double gain)
{
    int r = 0;
    if (dev->gain != gain)
    {
        r = rtlsdr_set_tuner_gain(dev->dev, (int)(gain * 10));
        if (r < 0)
        {
            ERROR("Failed to set tuner gain.\n");
        }
        else
        {
            INFO("Tuner gain set to %f dB.\n", gain);
            dev->gain = gain;
        }
    }
    return r;
}

uint32_t sensor_get_buffer_size()
{
    return dev->samples;
}

void sensor_start()
{
    DEBUG("Starting...\n");

    running = true;
    pthread_create(&worker_thread, NULL, worker, NULL);

    DEBUG("Sensor started.\n");
}

void sensor_stop()
{
    DEBUG("* Stopping worker_thread\n");

    running = false;
    pthread_join(worker_thread, NULL);
}

void sensor_close()
{
    free(buf);
    free(samples_output);

    pthread_mutex_destroy(&callback_mutex);

    DEBUG("sensor_close called with dev == %lx\n", (unsigned long)dev);
    if (dev->dev != NULL)
    {
        rtlsdr_close(dev->dev);
    }
    free(dev);
    DEBUG("sensor_close returns\n");
}