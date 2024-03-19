#ifndef AUDIO_H
#define AUDIO_H

#include "dsp_common.h"

void audio_init();

void audio_reset();

void audio_fm_demodulator(const cmplx_dbl* signal, int len);

int audio_new_audio_available();

int audio_get_audio_payload(char* buf, int buf_len);

void audio_close();

#endif