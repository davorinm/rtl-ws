#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include <iio.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include <time.h> 
#include <sys/time.h>

#include "sensor.h"
#include "sensor_pluto_helpers.h"
#include "../tools/helpers.h"
#include "../tools/timer.h"
#include "../settings.h"
#include "../dsp/spectrum.h"

/* IIO structs required for streaming */
static struct iio_context *ctx = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_buffer *rxbuf = NULL;

// Stream configurations
struct stream_cfg rxcfg;

// Streaming devices
struct iio_device *rx;

// Thread
static pthread_t worker_thread;
static bool running = false;

static int sensor_loop(fftw_complex *in_c)
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

        in_c[cnt] = (real + I * imag) / 2048;
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
    // Buffer
    unsigned int buffer_size = PLUTO_SAMPLES_PER_READ;

    // Vals
    int ret;

    // RX stream config
	rxcfg.bw_hz = MHZ(10);   // 2 MHz rf bandwidth
	rxcfg.fs_hz = MHZ(10);   // 2.5 MS/s rx sample rate
	rxcfg.lo_hz = MHZ(430); // 2.5 GHz rf frequency
    rxcfg.agc_mode = "slow_attack"; // "manual fast_attack slow_attack hybrid" 
    rxcfg.gain = 60;              // 0 dB gain
    rxcfg.rfport = "A_BALANCED"; // port A (select for rf freq.)
    // "A_BALANCED B_BALANCED C_BALANCED A_N A_P B_N B_P C_N C_P TX_MONITOR1 TX_MONITOR2 TX_MONITOR1_2" 

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
    ret = get_ad9361_stream_dev(ctx, RX, &rx);
    if (ret < 0)
    {
        DEBUG("No rx dev found\n");
        sensor_close();
        return -3;
    }

    DEBUG("* Configuring AD9361 for streaming\n");
    ret = cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);
    if (ret < 0)
    {
        DEBUG("RX port 0 not found\n");
        sensor_close();
        return -4;
    }

    DEBUG("* Initializing AD9361 IIO streaming channels\n");
    ret = get_ad9361_stream_ch(RX, rx, 0, &rx0_i);
    if (ret < 0)
    {
        DEBUG("RX chan i not found\n");
        sensor_close();
        return -5;
    }
    ret = get_ad9361_stream_ch(RX, rx, 1, &rx0_q);
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
    rxbuf = iio_device_create_buffer(rx, buffer_size, false);
    if (rxbuf == NULL)
    {
        DEBUG("Could not create RX buffer");
        sensor_close();
        return -7;
    }

    return 0;
}

uint32_t sensor_get_freq()
{
    return rxcfg.lo_hz;
}

int sensor_set_frequency(uint32_t f)
{
    if (rxcfg.lo_hz == f)
    {
        DEBUG("Frequency already set\n");
        return 0;
    }

    rxcfg.lo_hz = f;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);

    return ret;
}

uint32_t sensor_get_sample_rate()
{
    return rxcfg.fs_hz;
}

int sensor_set_sample_rate(uint32_t fs)
{
    if (rxcfg.fs_hz == fs)
    {
        DEBUG("Sample rate already set\n");
        return 0;
    }

    rxcfg.fs_hz = fs;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);

    return ret;
}

double sensor_get_gain()
{
    DEBUG("Sensor gain %2.1f dB\n", rxcfg.gain);
    return rxcfg.gain;
}

int sensor_set_gain(double gain)
{
    if (rxcfg.gain == gain)
    {
        DEBUG("Gain already set\n");
        return 0;
    }

    rxcfg.gain = gain;

    DEBUG("* Configuring AD9361 for streaming\n");
    int ret = cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);

    return ret;
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
}