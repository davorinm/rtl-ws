#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include <fftw3.h>

void dsp_init();

void dsp_close();

int dsp_process(fftw_complex *samples);

int dsp_spectrum_available();

int dsp_spectrum_get_payload(char *buf, int buf_len);

void dsp_audio_start();

void dsp_audio_stop();

int dsp_audio_available();

int dsp_audio_payload(char* buf, int buf_len);

#endif