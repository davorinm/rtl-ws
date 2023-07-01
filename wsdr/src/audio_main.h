#ifndef AUDIO_MAIN_H
#define AUDIO_MAIN_H

#include "dsp/dsp_common.h"

void audio_init();

void audio_reset();

int audio_new_audio_available();

int audio_get_audio_payload(char* buf, int buf_len);

void audio_fm_demodulator(const cmplx_s32* signal, int len);

void audio_close();

#endif