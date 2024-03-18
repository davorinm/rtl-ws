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
    int input_signal_count;

    cmplx_dbl *output_signal;
    int output_signal_len;
    int output_signal_count;

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

            decim->output_signal_len = target_rate;
            decim->input_signal_len = decim->output_signal_len * decim->down_factor;

            decim->output_signal = (cmplx_dbl *)realloc(decim->output_signal, decim->output_signal_len * sizeof(cmplx_dbl));
            decim->input_signal = (cmplx_dbl *)realloc(decim->input_signal, decim->input_signal_len * sizeof(cmplx_dbl));

            decim->output_signal_count = 0;
            decim->input_signal_count = 0;
        }
        r = 0;
    }
    pthread_mutex_unlock(&(decim->mutex));

    return r;
}

int rf_decimator_decimate(const cmplx_dbl *complex_signal, int complex_signal_len)
{
    pthread_mutex_lock(&(decim->mutex));

    // Copy data to processing array
    memcpy(decim->input_signal + decim->input_signal_count, complex_signal, complex_signal_len * sizeof(cmplx_dbl));
    decim->input_signal_count += complex_signal_len;

    pthread_mutex_unlock(&(decim->mutex));

    // Process at least one output
    if (decim->input_signal_count > decim->down_factor)
    {
        pthread_mutex_lock(&(decim->mutex));

        // Cleanup
        processed_input = 0;
        processed_output = 0;

        // Dcimate
        cic_decimate(decim->down_factor, decim->input_signal, decim->input_signal_count, decim->output_signal + decim->output_signal_count, decim->output_signal_len - decim->output_signal_count, &(decim->delay), &processed_input, &processed_output);

        // Update input indexes
        memcpy(decim->input_signal, decim->input_signal + processed_input, (decim->input_signal_count - processed_input) * sizeof(cmplx_dbl));
        decim->input_signal_count -= processed_input;

        // Update output indexes
        decim->output_signal_count += processed_output;

        pthread_mutex_unlock(&(decim->mutex));
    }

    // Can be processed, is full
    if (decim->output_signal_len == decim->output_signal_count)
    {
        // DEBUG("cic_decimate output_signal_len %d\n", output_signal_len);
        decim->callback(decim->output_signal, decim->output_signal_len);

        // Clear
        decim->output_signal_count = 0;
    }

    return 0;
}

void rf_decimator_free()
{
    pthread_mutex_destroy(&(decim->mutex));
    decim->callback = NULL;
    free(decim->input_signal);
    free(decim->output_signal);
    free(decim);
}