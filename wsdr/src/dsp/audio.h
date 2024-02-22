#ifndef AUDIO_MAIN_H
#define AUDIO_MAIN_H

#include <stdint.h>
#include <fftw3.h>

void audio_init();

void audio_reset();

void audio_close();

void audio_process(fftw_complex *samples);

int audio_available();

int audio_payload(char *buf, int buf_len);

void audio_fm_demodulate(fftw_complex *samples, int len);

#endif