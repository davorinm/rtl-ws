#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "rf_decimator.h"
#include "resample.h"
#include "rf_cic_decimate.h"
#include "../tools/helpers.h"

typedef struct rf_decimator
{
    pthread_mutex_t mutex;
    rf_decimator_callback callback;

    int sample_rate;
    int down_factor;

    cmplx_dbl *input_signal;
    int input_signal_len;
    int surplus;

    cmplx_dbl *resampled_signal;
    int resampled_signal_len;

    struct cic_delay_line delay;
} rf_decimator;

static rf_decimator *decim = NULL;

static int processed_input = 0;
static int processed_output = 0;

void rf_decimator_init(rf_decimator_callback callback)
{
    decim = (rf_decimator *)calloc(1, sizeof(rf_decimator));
    pthread_mutex_init(&(decim->mutex), NULL);
    decim->callback = callback;
}

int rf_decimator_set_parameters(int sample_rate, int buffer_size, int target_rate)
{
    int r = -1;

    pthread_mutex_lock(&(decim->mutex));
    if (sample_rate > 0 && target_rate > 0)
    {
        int down_factor = sample_rate / target_rate;

        if (decim->sample_rate != sample_rate || decim->down_factor != down_factor)
        {
            DEBUG("Setting RF decimator params: sample_rate == %d, down_factor == %d\n", sample_rate, down_factor);
            decim->sample_rate = sample_rate;
            decim->down_factor = down_factor;
            decim->resampled_signal_len = 20000;
            decim->input_signal_len = decim->resampled_signal_len * decim->down_factor;

            decim->resampled_signal = (cmplx_dbl *)realloc(decim->resampled_signal, decim->resampled_signal_len * sizeof(cmplx_dbl));
            decim->input_signal = (cmplx_dbl *)realloc(decim->input_signal, decim->input_signal_len * sizeof(cmplx_dbl));

            decim->surplus = 0;
        }
        r = 0;
    }
    pthread_mutex_unlock(&(decim->mutex));

    return r;
}

int rf_decimator_decimate(const cmplx_dbl *complex_signal, int len)
{
    int current_idx = 0;
    int remaining = len;
    int block_size = 0;

    pthread_mutex_lock(&(decim->mutex));

    block_size = decim->input_signal_len - decim->surplus;

    if (decim->resampled_signal == NULL || decim->input_signal == NULL)
        return -1;

    while (remaining >= block_size)
    {
        memcpy(&(decim->input_signal[decim->surplus]), &(complex_signal[current_idx]), block_size * sizeof(cmplx_dbl));
        remaining -= block_size;
        current_idx += block_size;

        cic_decimate(decim->down_factor, decim->input_signal, decim->input_signal_len, decim->resampled_signal, decim->resampled_signal_len, &(decim->delay), &processed_input, &processed_output);

        // Callbback
        decim->callback(decim->resampled_signal, decim->resampled_signal_len);

        decim->surplus = 0;
        block_size = decim->input_signal_len;
    }

    if (remaining > 0)
    {
        memcpy(&(decim->input_signal[decim->surplus]), &(complex_signal[len - remaining]), remaining * sizeof(cmplx_dbl));
        decim->surplus += remaining;
    }
    pthread_mutex_unlock(&(decim->mutex));

    return 0;
}

void rf_decimator_free()
{
    pthread_mutex_destroy(&(decim->mutex));
    decim->callback = NULL;
    free(decim->input_signal);
    free(decim->resampled_signal);
    free(decim);
}