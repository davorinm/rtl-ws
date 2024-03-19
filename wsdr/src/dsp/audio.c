#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "audio.h"
#include "resample.h"
#include "../tools/helpers.h"

static float *demod_buffer = NULL;
static int demod_buffer_len = 0;

static float *work_audio_buffer = NULL;
static int work_audio_buffer_len = 0;

static float *play_audio_buffer = NULL;
static int play_audio_buffer_len = 0;

static float *audio_buffer = NULL;
static int audio_buffer_len = 0;
static int audio_buffer_count = 0;

static pthread_mutex_t audio_mutex;

void audio_init()
{
    pthread_mutex_init(&audio_mutex, NULL);
}

void audio_reset()
{
    pthread_mutex_lock(&audio_mutex);

    demod_buffer_len = 0;

    pthread_mutex_unlock(&audio_mutex);
}

void audio_fm_demodulator(const cmplx_dbl *signal, int len)
{
    static const float scale = 1;
    static float delay_line_1[HALF_BAND_N - 1] = {0};
    static float delay_line_2[HALF_BAND_N - 1] = {0};
    static float prev_sample = 0;
    float temp = 0;
    int i = 0;

    if (demod_buffer_len != len)
    {
        DEBUG("Reallocating demodulation and audio buffers %d\n", len);

        pthread_mutex_lock(&audio_mutex);

        demod_buffer_len = len;
        demod_buffer = (float *)realloc(demod_buffer, demod_buffer_len * sizeof(float));

        work_audio_buffer_len = len / 2;
        work_audio_buffer = (float *)realloc(work_audio_buffer, work_audio_buffer_len * sizeof(float));

        play_audio_buffer_len = work_audio_buffer_len / 2;
        play_audio_buffer = (float *)realloc(play_audio_buffer, play_audio_buffer_len * sizeof(float));

        audio_buffer_len = len * 2;
        audio_buffer = (float *)realloc(audio_buffer, audio_buffer_len * sizeof(float));
        audio_buffer_count = 0;

        pthread_mutex_unlock(&audio_mutex);
    }

    for (i = 0; i < len; i++)
    {
        demod_buffer[i] = atan2_approx(imag_cmplx_s32(signal[i]), real_cmplx_s32(signal[i]));

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

    halfband_decimate(work_audio_buffer, play_audio_buffer, play_audio_buffer_len, delay_line_2);

    // Check space
    if (audio_buffer_len - audio_buffer_count < play_audio_buffer_len)
    {
        // Make space by shifting
        memcpy(audio_buffer, audio_buffer + play_audio_buffer_len, (audio_buffer_count - play_audio_buffer_len) * sizeof(float));
    }

    // Move to audio
    memcpy(audio_buffer + audio_buffer_count, play_audio_buffer, play_audio_buffer_len * sizeof(float));
    audio_buffer_count += play_audio_buffer_len;

    pthread_mutex_unlock(&audio_mutex);
}

int audio_new_audio_available()
{
    return audio_buffer_count > 0;
}

int audio_get_audio_payload(char *buf, int buf_len)
{
    pthread_mutex_lock(&audio_mutex);

    int buf_len_samples = buf_len / sizeof(float);
    int copied_samples = MIN(buf_len_samples - 2, audio_buffer_count);

    // Copy to output buffer
    memcpy(buf, audio_buffer, copied_samples * sizeof(float));
    audio_buffer_count -= copied_samples;

    // Move used data
    memcpy(audio_buffer, audio_buffer + copied_samples, audio_buffer_count * sizeof(float));

    pthread_mutex_unlock(&audio_mutex);

    return copied_samples * sizeof(float);
}

void audio_close()
{
    pthread_mutex_destroy(&audio_mutex);

    free(demod_buffer);
    free(work_audio_buffer);
    free(play_audio_buffer);
    free(audio_buffer);
}