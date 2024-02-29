#ifndef AUDIO_MAIN_H
#define AUDIO_MAIN_H

#include <stdint.h>
#include "dsp_common.h"

void audio_init();

void audio_reset();

int audio_available();

int audio_payload(char *buf, int buf_len);

void audio_process(const cmplx_s32 *signal, int len);

void audio_close();

#endif