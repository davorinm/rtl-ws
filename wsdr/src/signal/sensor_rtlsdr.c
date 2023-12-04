#ifdef SENSOR_RTLSDR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rtl-sdr.h>

#include "sensor_rtlsdr.h"
#include "../tools/helpers.h"
#include "../settings.h"

#define RTL_DEFAULT_DEVICE_INDEX 0
#define RTL_DEFAULT_GAIN 25.4

typedef struct rtl_dev
{
    rtlsdr_dev_t *dev;
    uint32_t f;
    uint32_t fs;
    double gain;
} rtl_dev_t;

static rtl_dev_t *dev = NULL;

int rtl_init()
{
    int r = 0;
    DEBUG("rtl_init called with dev == %lx, dev_index == %d\n", (unsigned long)dev, RTL_DEFAULT_DEVICE_INDEX);
    // rtl_close();
    // New
    dev = (rtl_dev_t *)calloc(1, sizeof(rtl_dev_t));

    // Open
    DEBUG("rtlsdr_open...\n");
    r = rtlsdr_open(&dev->dev, RTL_DEFAULT_DEVICE_INDEX);
    if (r < 0)
    {
        ERROR("Failed to open rtlsdr device #%d.\n", RTL_DEFAULT_DEVICE_INDEX);
        return r;
    }

    DEBUG("rtl_set_sample_rate...\n");
    r = rtl_set_sample_rate(RTL_DEFAULT_SAMPLE_RATE);
    if (r < 0)
    {
        ERROR("Failed to set sample rate \n");
        return r;
    }

    DEBUG("rtl_set_frequency...\n");
    r = rtl_set_frequency(RTL_DEFAULT_FREQUENCY);
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
    r = rtl_set_gain(RTL_DEFAULT_GAIN);
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

int rtl_set_frequency(uint32_t f)
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

int rtl_set_sample_rate(uint32_t fs)
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

int rtl_set_gain(double gain)
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

uint32_t rtl_freq()
{
    return dev->f;
}

uint32_t rtl_sample_rate()
{
    return dev->fs;
}

double rtl_gain()
{
    return dev->gain;
}

int rtl_read_async(void (*callback)(unsigned char *, uint32_t, void *), void *user)
{
    return rtlsdr_read_async(dev->dev, (rtlsdr_read_async_cb_t)callback, user, 0, 0);
}

void rtl_cancel()
{
    if (dev->dev != NULL)
    {
        if (rtlsdr_cancel_async(dev->dev) != 0)
        {
            ERROR("Canceling rtl async failed\n");
        }
    }
}

void rtl_close()
{
    DEBUG("rtl_close called with dev == %lx\n", (unsigned long)dev);
    if (dev->dev != NULL)
    {
        rtlsdr_close(dev->dev);
    }
    free(dev);
    DEBUG("rtl_close returns\n");
}

#endif