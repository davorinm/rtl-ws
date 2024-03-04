#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <iio.h>

#include <math.h>

#include <time.h>
#include <sys/time.h>

#include "sensor.h"
#include "sensor_pluto_helpers.h"
#include "../dsp/dsp.h"
#include "../tools/helpers.h"
#include "../tools/timer.h"

/* IIO structs required for streaming */
static struct iio_context *ctx = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_buffer *rxbuf = NULL;

// Stream configurations
struct sensor_config config;

// Streaming devices
struct iio_device *rx;

// Buffer
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
    callback_function(samples_output, config.buffer_size);
    pthread_mutex_unlock(&callback_mutex);
}

static int sensor_loop()
{
    ssize_t nbytes_rx;
    char *p_dat, *p_end;
    ptrdiff_t p_inc;
    unsigned int cnt;

    // Refill RX buffer
    nbytes_rx = iio_buffer_refill(rxbuf);
    if (nbytes_rx < 0)
    {
        DEBUG("Error refilling buf %d\n", (int)nbytes_rx);
        return -1;
    }

    // DEBUG("Refilling buf with %d bytes\n", (int)nbytes_rx);

    // READ: Get pointers to RX buf and read IQ from RX buf port 0
    p_inc = iio_buffer_step(rxbuf);
    p_end = iio_buffer_end(rxbuf);

    for (cnt = 0, p_dat = (char *)iio_buffer_first(rxbuf, rx0_i); p_dat < p_end; p_dat += p_inc, cnt++)
    {
        const int16_t real = ((int16_t *)p_dat)[0]; // Real (I)
        const int16_t imag = ((int16_t *)p_dat)[1]; // Imag (Q)

        // in_c[cnt] = (real + I * imag) / 2048;
        // TODO: Make real cplx number

        samples_output[cnt].p.re = (int32_t)real - 2048;
        samples_output[cnt].p.im = (int32_t)imag - 2048;
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
    int ret;

    // RX stream config
	config.bw_hz = MHZ(10);             // 2 MHz rf bandwidth
	config.fs_hz = MHZ(10);             // 2.5 MS/s rx sample rate
    config.gain_mode = 0;               // "manual fast_attack slow_attack hybrid" 
    config.gain = 60;                   // 0 dB gain
    config.rfport = "A_BALANCED";       // port A (select for rf freq.)
    config.buffer_size = 1024 * 8;      // Device samples buffer size
    config.lo_hz = MHZ(100);       // 2.5 GHz rf frequency

    // Buffer
    samples_output = calloc(1, config.buffer_size * sizeof(cmplx_s32));

    // Lock
    pthread_mutex_init(&callback_mutex, NULL);

    DEBUG("* Acquiring IIO context\n");
    ctx = iio_create_default_context();
    if (ctx == NULL)
    {
        DEBUG("No context\n");
        sensor_close();
        return -1;
    }

    DEBUG("* Acquiring IIO devices\n");
    ret = iio_context_get_devices_count(ctx);
    if (ret < 0)
    {
        DEBUG("No devices\n");
        sensor_close();
        return -12;
    }
    DEBUG("Found %i devices\n", ret);

    DEBUG("* Acquiring AD9361 streaming devices\n");
    ret = get_ad9361_rx_stream_dev(ctx, &rx);
    if (ret < 0)
    {
        DEBUG("No rx dev found\n");
        sensor_close();
        return -3;
    }

    DEBUG("* Configuring AD9361 for streaming\n");
    ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);
    if (ret < 0)
    {
        DEBUG("RX port 0 not found\n");
        sensor_close();
        return -4;
    }

    DEBUG("* Initializing AD9361 IIO streaming channels\n");
    ret = get_ad9361_rx_stream_ch(rx, 0, &rx0_i);
    if (ret < 0)
    {
        DEBUG("RX chan i not found\n");
        sensor_close();
        return -5;
    }
    ret = get_ad9361_rx_stream_ch(rx, 1, &rx0_q);
    if (ret < 0)
    {
        DEBUG("RX chan q not found\n");
        sensor_close();
        return -6;
    }

    DEBUG("* Enabling IIO streaming channels\n");
    iio_channel_enable(rx0_i);
    iio_channel_enable(rx0_q);

    DEBUG("* Creating non-cyclic IIO buffers\n");
    rxbuf = iio_device_create_buffer(rx, config.buffer_size, false);
    if (rxbuf == NULL)
    {
        DEBUG("Could not create RX buffer");
        sensor_close();
        return -7;
    }

    return 0;
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
    return config.lo_hz;
}

int sensor_set_frequency(uint32_t f)
{
    if (config.lo_hz == f)
    {
        DEBUG("Frequency already set\n");
        return 0;
    }

    config.lo_hz = f;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);

    return ret;
}

uint32_t sensor_get_rf_band_width()
{
    return config.bw_hz;
}

int sensor_set_rf_band_width(uint32_t bw)
{
    if (config.bw_hz == bw)
    {
        DEBUG("Sample rate already set\n");
        return 0;
    }

    config.bw_hz = bw;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);

    return ret;
}

uint32_t sensor_get_sample_rate()
{
    return config.fs_hz;
}

int sensor_set_sample_rate(uint32_t fs)
{
    if (config.fs_hz == fs)
    {
        DEBUG("Sample rate already set\n");
        return 0;
    }

    config.fs_hz = fs;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);

    return ret;
}

uint32_t sensor_get_gain_mode()
{
    return config.gain_mode;
}

int sensor_set_gain_mode(uint32_t gain_mode)
{

    if (config.gain_mode == gain_mode)
    {
        DEBUG("Gain mode already set\n");
        return 0;
    }

    config.gain_mode = gain_mode;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);

    return ret;
}

double sensor_get_gain()
{
    return config.gain;
}

int sensor_set_gain(double gain)
{
    if (config.gain == gain)
    {
        DEBUG("Gain already set\n");
        return 0;
    }

    config.gain = gain;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_rx_stream_ch(ctx, &config, 0);

    return ret;
}

uint32_t sensor_get_buffer_size()
{
    return config.buffer_size;
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
    DEBUG("* Stopping sensor\n");
    sensor_stop();

    DEBUG("* Destroying buffers\n");
    if (rxbuf)
    {
        iio_buffer_destroy(rxbuf);
    }

    DEBUG("* Disabling streaming channels\n");
    if (rx0_i)
    {
        iio_channel_disable(rx0_i);
    }
    if (rx0_q)
    {
        iio_channel_disable(rx0_q);
    }

    DEBUG("* Destroying context\n");
    if (ctx)
    {
        iio_context_destroy(ctx);
    }

    free(samples_output);

    pthread_mutex_destroy(&callback_mutex);
}