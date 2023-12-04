#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "sensor.h"
#include "../tools/list.h"
#include "../tools/helpers.h"

#ifdef SENSOR_RTLSDR
#include "sensor_rtlsdr.h"
#elif SENSOR_PLUTO
#include "sensor_pluto.h"
#endif

struct _signal_holder
{
    const cmplx_u8 *signal;
    int len;
};

static pthread_t worker_thread;
static pthread_mutex_t callback_mutex;
static struct list *callback_list = NULL;
static volatile int running = 0;

static void signal_source_callback_notifier(void *callback, void *signal)
{
    signal_source_callback f = (signal_source_callback)callback;
    struct _signal_holder *s_h = (struct _signal_holder *)signal;
    f(s_h->signal, s_h->len);
}

static void sensor_async_callback(unsigned char *buf, uint32_t len, void *user)
{
    // DEBUG("sensor_async_callback len: %d\n", len);

    struct rtl_dev *dev_ctx = (struct rtl_dev *)user;
    UNUSED(dev_ctx);

    struct _signal_holder s_h = {(cmplx_u8 *)buf, len / 2};
    pthread_mutex_lock(&callback_mutex);
    list_apply2(callback_list, signal_source_callback_notifier, &s_h);
    pthread_mutex_unlock(&callback_mutex);
}

static void *worker(void *user)
{
    int status = 0;
    DEBUG("Reading signal from sensor\n");
    if (running)
    {
#ifdef SENSOR_RTLSDR
        status = rtl_read_async(sensor_async_callback, user);
#elif SENSOR_PLUTO
        status = pluto_read_async(sensor_async_callback, user);
#endif
        if (status < 0)
        {
            ERROR("Read failed with status %d\n", status);
        }
    }

    DEBUG("Done reading signal from sensor\n");

    return NULL;
}

// ================

int sensor_init()
{
#ifdef SENSOR_RTLSDR
    return rtl_init();
#elif SENSOR_PLUTO
    return pluto_init();
#endif
}

uint32_t sensor_get_freq()
{
#ifdef SENSOR_RTLSDR
    return rtl_freq();
#elif SENSOR_PLUTO
    return pluto_freq();
#endif
}

int sensor_set_frequency(uint32_t f)
{
#ifdef SENSOR_RTLSDR
    return rtl_set_frequency(f);
#elif SENSOR_PLUTO
    return pluto_set_frequency(f);
#endif
}

uint32_t sensor_get_sample_rate()
{
#ifdef SENSOR_RTLSDR
    return rtl_sample_rate();
#elif SENSOR_PLUTO
    return pluto_sample_rate();
#endif
}

int sensor_set_sample_rate(uint32_t fs)
{
#ifdef SENSOR_RTLSDR
    return rtl_set_sample_rate(fs);
#elif SENSOR_PLUTO
    return pluto_set_sample_rate(fs);
#endif
}

int sensor_set_gain(double gain) {
#ifdef SENSOR_RTLSDR
    return rtl_set_gain(gain);
#elif SENSOR_PLUTO
    return pluto_set_gain(gain);
#endif
}

double sensor_get_gain() {
#ifdef SENSOR_RTLSDR
    return rtl_gain();
#elif SENSOR_PLUTO
    return pluto_gain();
#endif
}

void signal_source_add_callback(signal_source_callback callback)
{
    pthread_mutex_lock(&callback_mutex);
    list_add(callback_list, callback);
    pthread_mutex_unlock(&callback_mutex);
}

void signal_source_remove_callbacks()
{
    pthread_mutex_lock(&callback_mutex);
    list_clear(callback_list);
    pthread_mutex_unlock(&callback_mutex);
}

void sensor_start()
{
    DEBUG("Starting signal source...\n");
    if (running)
    {
        return;
    }

    running = 1;
    callback_list = list_alloc();
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_create(&worker_thread, NULL, worker, NULL);
    DEBUG("Signal source started.\n");
}

void sensor_stop()
{
    DEBUG("Stopping signal source...\n");
    if (!running)
    {
        return;
    }

    running = 0;

#ifdef SENSOR_RTLSDR
    rtl_cancel();
#elif SENSOR_PLUTO
    pluto_cancel();
#endif

    pthread_join(worker_thread, NULL);
    pthread_mutex_destroy(&callback_mutex);
    list_free(callback_list);
    DEBUG("Signal source stopped.\n");
}

void sensor_close()
{
    DEBUG("Closing signal source...\n");
#ifdef SENSOR_RTLSDR
    rtl_close();
#elif SENSOR_PLUTO
    pluto_close();
#endif
}