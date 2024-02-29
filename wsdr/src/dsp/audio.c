#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "audio.h"
#include "resample.h"
#include "../tools/helpers.h"
#include "../tools/list.h"

#define AUDIO_BUFFER_POOL 50

static float *demod_buffer = NULL;
static int demod_buffer_len = 0;
static float *work_audio_buffer = NULL;
static int work_audio_buffer_len = 0;

static struct list *full_audio_buffers = NULL;
static struct list *used_audio_buffers = NULL;

static int audio_buffer_len = 0;
static pthread_mutex_t audio_mutex;

static float atan2_approx(float y, float x)
{
    static const float pi_by_2 = (float)(M_PI / 2);
    register float atan = 0;
    register float z = 0;

    if (x == 0)
    {
        if (y > 0.0f)
            return pi_by_2;

        if (y == 0)
            return 0;

        return -pi_by_2;
    }
    z = y / x;
    if (fabs(z) < 1.0f)
    {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0)
        {
            if (y < 0.0f)
                return atan - M_PI;

            return atan + M_PI;
        }
    }
    else
    {
        atan = pi_by_2 - z / (z * z + 0.28f);
        if (y < 0.0f)
            return atan - M_PI;
    }
    return atan;
}

void audio_init()
{
    used_audio_buffers = list_alloc();
    full_audio_buffers = list_alloc();
    pthread_mutex_init(&audio_mutex, NULL);
}

void audio_reset()
{
    pthread_mutex_lock(&audio_mutex);

    list_apply(full_audio_buffers, free);
    list_apply(used_audio_buffers, free);
    list_clear(full_audio_buffers);
    list_clear(used_audio_buffers);

    demod_buffer_len = 0;
    work_audio_buffer_len = 0;
    audio_buffer_len = 0;

    pthread_mutex_unlock(&audio_mutex);
}

int audio_available()
{
    if (full_audio_buffers == NULL)
    {
        return 0;
    }

    return list_length(full_audio_buffers) > 0;
}

int audio_payload(char *buf, int buf_len)
{
    static int audio_buf_cur_idx = 0;
    float *data = NULL;
    int new_samples = 0;
    int copied_samples = 0;
    int buf_samples_remaining = buf_len / sizeof(float);

    do
    {
        pthread_mutex_lock(&audio_mutex);
        data = list_peek(full_audio_buffers);
        if (data != NULL)
        {
            if (audio_buf_cur_idx >= audio_buffer_len)
            {
                list_poll_to_list(full_audio_buffers, used_audio_buffers);
                audio_buf_cur_idx = 0;
            }
            new_samples = audio_buffer_len - audio_buf_cur_idx;
            new_samples = (new_samples <= buf_samples_remaining) ? new_samples : buf_samples_remaining;
            if (new_samples > 0)
            {
                memcpy(&(buf[copied_samples]), &(data[audio_buf_cur_idx]), new_samples * sizeof(float));
                audio_buf_cur_idx += new_samples;
                copied_samples += new_samples;
                buf_samples_remaining -= new_samples;
            }
        }
        pthread_mutex_unlock(&audio_mutex);
    } while (data != NULL && buf_samples_remaining > 0);

    return copied_samples * sizeof(float);
}

void audio_process(fftw_complex *samples, int len)
{
    static const float scale = 1;
    static float delay_line_1[HALF_BAND_N - 1] = {0};
    static float delay_line_2[HALF_BAND_N - 1] = {0};
    static float prev_sample = 0;
    float temp = 0;
    int i = 0;

    if (demod_buffer_len != len)
    {
        DEBUG("Reallocating demodulation and audio buffers\n");

        demod_buffer_len = len;
        demod_buffer = (float *)realloc(demod_buffer, demod_buffer_len * sizeof(float));

        work_audio_buffer_len = len / 2;
        work_audio_buffer = (float *)realloc(work_audio_buffer, work_audio_buffer_len * sizeof(float));

        pthread_mutex_lock(&audio_mutex);

        list_apply(full_audio_buffers, free);
        list_apply(used_audio_buffers, free);
        list_clear(full_audio_buffers);
        list_clear(used_audio_buffers);

        audio_buffer_len = work_audio_buffer_len / 2;

        for (i = 0; i < AUDIO_BUFFER_POOL; i++)
        {
            list_add(used_audio_buffers, calloc(audio_buffer_len, sizeof(float)));
        }

        pthread_mutex_unlock(&audio_mutex);
    }

    for (i = 0; i < len; i++)
    {
        demod_buffer[i] = atan2_approx(samples[i][1], samples[i][0]);

        temp = demod_buffer[i];
        demod_buffer[i] -= prev_sample;
        prev_sample = temp;

        // hard limit output
        if (demod_buffer[i] > scale)
        {
            demod_buffer[i] = 1;
        }
        else if (demod_buffer[i] < -scale)
        {
            demod_buffer[i] = -1;
        }
        else
        {
            demod_buffer[i] /= scale;
        }
    }

    halfband_decimate(demod_buffer, work_audio_buffer, work_audio_buffer_len, delay_line_1);

    pthread_mutex_lock(&audio_mutex);

    if (list_length(used_audio_buffers) > 0)
    {
        halfband_decimate(work_audio_buffer, list_peek(used_audio_buffers), audio_buffer_len, delay_line_2);
        list_poll_to_list(used_audio_buffers, full_audio_buffers);
    }

    pthread_mutex_unlock(&audio_mutex);
}

void audio_close()
{
    pthread_mutex_destroy(&audio_mutex);

    free(demod_buffer);
    free(work_audio_buffer);

    list_apply(used_audio_buffers, free);
    list_clear(used_audio_buffers);

    list_apply(full_audio_buffers, free);
    list_clear(full_audio_buffers);
}