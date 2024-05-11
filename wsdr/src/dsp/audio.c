#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "audio.h"
#include "demodulator.h"
#include "rf_cic_decimate.h"
#include "../tools/helpers.h"

static float *demod_buffer = NULL;
static int demod_buffer_len = 0;

static float *play_audio_buffer = NULL;
static int play_audio_buffer_len = 0;

static float *audio_buffer = NULL;
static int audio_buffer_len = 0;
static int audio_buffer_count = 0;

static pthread_mutex_t audio_mutex;
static int audio_running = 0;

static float delay_line_2[HALF_BAND_N - 1] = {0};

static const float scale = 1;

void audio_init()
{
    pthread_mutex_init(&audio_mutex, NULL);
}

void audio_start()
{
    pthread_mutex_lock(&audio_mutex);

    audio_buffer_count = 0;
    audio_running = 1;

    pthread_mutex_unlock(&audio_mutex);
}

void audio_stop()
{
    pthread_mutex_lock(&audio_mutex);

    audio_buffer_count = 0;
    audio_running = 0;

    pthread_mutex_unlock(&audio_mutex);
}

void audio_process(const cmplx_dbl *signal, int len)
{
    if (audio_running == 0) {
        return;
    }

    if (demod_buffer_len != len)
    {
        DEBUG("Reallocating demodulation and audio buffers %d %d\n", demod_buffer_len, len);

        pthread_mutex_lock(&audio_mutex);

        demod_buffer_len = len;
        demod_buffer = (float *)realloc(demod_buffer, demod_buffer_len * sizeof(float));

        play_audio_buffer_len = len / 2;
        play_audio_buffer = (float *)realloc(play_audio_buffer, play_audio_buffer_len * sizeof(float));

        audio_buffer_len = len * 2;
        audio_buffer = (float *)realloc(audio_buffer, audio_buffer_len * sizeof(float));
        audio_buffer_count = 0;

        pthread_mutex_unlock(&audio_mutex);
    }

    pthread_mutex_lock(&audio_mutex);

    fm_demodulator(signal, demod_buffer, len, scale);

    rf_halfband_decimate(demod_buffer, play_audio_buffer, play_audio_buffer_len, delay_line_2);

    // Check space
    if (audio_buffer_len - audio_buffer_count < play_audio_buffer_len)
    {
        // Make space by shifting for play_audio_buffer_len
        memcpy(audio_buffer, audio_buffer + play_audio_buffer_len, (audio_buffer_count - play_audio_buffer_len) * sizeof(float));
        audio_buffer_count -= play_audio_buffer_len;
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
    free(play_audio_buffer);
    free(audio_buffer);
}