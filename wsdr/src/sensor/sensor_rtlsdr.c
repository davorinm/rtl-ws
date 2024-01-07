#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <rtl-sdr.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include <stdbool.h>

#include "sensor.h"
#include "../tools/helpers.h"
#include "../tools/timer.h"
#include "../settings.h"
#include "../dsp/spectrum.h"

#define RTL_DEFAULT_DEVICE_INDEX 0
#define RTL_DEFAULT_GAIN 25.4

typedef struct rtl_dev
{
    rtlsdr_dev_t *dev;
    uint32_t f;
    uint32_t fs;
    double gain;
    uint32_t samples;
} rtl_dev_t;

static rtl_dev_t *dev = NULL;

static uint8_t *buf = NULL; 

// Thread
static pthread_t worker_thread;
static bool running = false;

static int sensor_loop(fftw_complex *in_c)
{
    int samples_read;
    int retval;

    // Refill RX buffer
    retval = rtlsdr_read_sync(dev->dev, buf, dev->samples, &samples_read);
    if (retval < 0)
    {
        DEBUG("Samples read failed: %d\n", retval);
        return 0;
    }

    // convert buffer from IQ to complex ready for FFTW, seems that rtlsdr outputs IQ data as IQIQIQIQIQIQ so ......
    int i;
    int n = 0;
    for (i = 0; i < (samples_read - 2); i += 2)
    {
        __real__ in_c[i] = buf[n] - 127; // sample is 127 for zero signal,so 127 +/-127
        __imag__ in_c[i] = buf[n + 1] - 127;
        // printf("%f + %f\n", __real__  in[i], __imag__ in[i]);
        n++;
    }

    return 0;
}

static void *worker(void *user)
{
    UNUSED(user);

    // struct timespec time;
    // double time_spent;

    int status = 0;
    DEBUG("Reading signal from sensor\n");

    while (running)
    {
        // timer_start(&time);

        /// Pointer to fft data
        fftw_complex *in_c = spectrum_data_input();

        status = sensor_loop(in_c);
        if (status < 0)
        {
            ERROR("Read failed with status %d\n", status);
        }

        // timer_end(&time, &time_spent);
        // DEBUG("Time elpased gathering is %f seconds\n", time_spent);

        // timer_start(&time);
        spectrum_process();

        // timer_end(&time, &time_spent);
        // DEBUG("Time elpased processing is %f seconds\n", time_spent);
    }

    DEBUG("Done reading signal from sensor\n");

    return 0;
}

int sensor_init()
{
    int r = 0;
    DEBUG("rtl_init called with dev == %lx, dev_index == %d\n", (unsigned long)dev, RTL_DEFAULT_DEVICE_INDEX);

    // Device
    dev = (rtl_dev_t *)calloc(1, sizeof(rtl_dev_t));
    dev->samples = 512;

    // Buffer
    buf = calloc(1, dev->samples * sizeof(uint8_t));

    // Open
    DEBUG("rtlsdr_open...\n");
    r = rtlsdr_open(&dev->dev, RTL_DEFAULT_DEVICE_INDEX);
    if (r < 0)
    {
        ERROR("Failed to open rtlsdr device #%d.\n", RTL_DEFAULT_DEVICE_INDEX);
        return r;
    }

    DEBUG("rtl_set_sample_rate...\n");
    r = sensor_set_sample_rate(RTL_DEFAULT_SAMPLE_RATE);
    if (r < 0)
    {
        ERROR("Failed to set sample rate \n");
        return r;
    }

    DEBUG("rtl_set_frequency...\n");
    r = sensor_set_frequency(RTL_DEFAULT_FREQUENCY);
    if (r < 0)
    {
        ERROR("Failed to set frequency \n");
        return r;
    }

    DEBUG("Setting rtl gain mode to manual...\n");
    r = rtlsdr_set_tuner_gain_mode(dev->dev, 1);
    if (r < 0)
    {
        ERROR("Failed to enable manual gain.\n");
        return r;
    }

    DEBUG("Setting rtl gain to %f...\n", RTL_DEFAULT_GAIN);
    r = sensor_set_gain(RTL_DEFAULT_GAIN);
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

uint32_t sensor_get_band_width() {
    return 2000000;
}

int sensor_set_band_width(uint32_t bw) {
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

uint32_t sensor_get_buffer_size() {
    return 0;
}

int sensor_set_buffer_size(uint32_t fs) {
    return 0;
}

void sensor_start()
{
    DEBUG("Starting...\n");

    running = true;
    pthread_create(&worker_thread, NULL, worker, NULL);

    DEBUG("Sensor started.\n");
}

void sensor_stop() {
    DEBUG("* Stopping worker_thread\n");
    
    running = false;
    pthread_join(worker_thread, NULL);
}

void sensor_close()
{
    free(buf);

    DEBUG("sensor_close called with dev == %lx\n", (unsigned long)dev);
    if (dev->dev != NULL)
    {
        rtlsdr_close(dev->dev);
    }
    free(dev);
    DEBUG("sensor_close returns\n");
}