#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "decimator.h"
#include "resample.h"
#include "cic_decimate.h"
#include "../tools/helpers.h"

#define INTERNAL_BUF_LEN_MS 100

struct rf_decimator
{
    pthread_mutex_t mutex;
    rf_decimator_callback callback;

    int sample_rate;
    int down_factor;

    cmplx_s32 *input_signal;
    int input_signal_len;
    int surplus;

    cmplx_s32 *resampled_signal;
    int resampled_signal_len;

    struct cic_delay_line delay;
};

rf_decimator *decimator_init(rf_decimator_callback callback)
{
    rf_decimator *d = (rf_decimator *)calloc(1, sizeof(rf_decimator));
    pthread_mutex_init(&(d->mutex), NULL);
    d->callback = callback;
    return d;
}

int rf_decimator_set_parameters(rf_decimator *d, int sample_rate, int down_factor)
{
    int r = -1;

    pthread_mutex_lock(&(d->mutex));
    if (sample_rate > 0 && down_factor > 0)
    {
        if (d->sample_rate != sample_rate || d->down_factor != down_factor)
        {
            DEBUG("Setting RF decimator params: sample_rate == %d, down_factor == %d\n", sample_rate, down_factor);
            d->sample_rate = sample_rate;
            d->down_factor = down_factor;
            d->resampled_signal_len = (int)((d->sample_rate / d->down_factor) * INTERNAL_BUF_LEN_MS / 1000);
            d->input_signal_len = d->resampled_signal_len * d->down_factor;

            d->resampled_signal = (cmplx_s32 *)realloc(d->resampled_signal, d->resampled_signal_len * sizeof(cmplx_s32));
            d->input_signal = (cmplx_s32 *)realloc(d->input_signal, d->input_signal_len * sizeof(cmplx_s32));

            d->surplus = 0;
        }
        r = 0;
    }
    pthread_mutex_unlock(&(d->mutex));

    return r;
}

int rf_decimator_decimate(rf_decimator *d, const cmplx_s32 *complex_signal, int len)
{
    int current_idx = 0;
    int remaining = len;
    int block_size = 0;

    pthread_mutex_lock(&(d->mutex));

    block_size = d->input_signal_len - d->surplus;

    if (d->resampled_signal == NULL || d->input_signal == NULL)
    {
        return -1;
    }

    while (remaining >= block_size)
    {
        memcpy(&(d->input_signal[d->surplus]), &(complex_signal[current_idx]), block_size * sizeof(cmplx_s32));
        remaining -= block_size;
        current_idx += block_size;

        if (cic_decimate(d->down_factor, d->input_signal, d->input_signal_len, d->resampled_signal, d->resampled_signal_len, &(d->delay)))
        {
            ERROR("Error while decimating signal\n");
            return -2;
        }

        // Callback
        rf_decimator_callback f = (rf_decimator_callback) d->callback;
        f(d->resampled_signal, d->resampled_signal_len);

        d->surplus = 0;
        block_size = d->input_signal_len;
    }

    if (remaining > 0)
    {
        memcpy(&(d->input_signal[d->surplus]), &(complex_signal[len - remaining]), remaining * sizeof(cmplx_s32));
        d->surplus += remaining;
    }
    pthread_mutex_unlock(&(d->mutex));

    return 0;
}

void rf_decimator_free(rf_decimator *d)
{
    d->callback = NULL;
    pthread_mutex_destroy(&(d->mutex));
    free(d->input_signal);
    free(d->resampled_signal);
    free(d);
}