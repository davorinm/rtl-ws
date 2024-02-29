#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include <fftw3.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define DECIMATED_TARGET_BW_HZ 192000

void dsp_init();

void dsp_close();

fftw_complex *dsp_samples();

void dsp_process();

int dsp_spectrum_available();

int dsp_spectrum_get_payload(char *buf, int buf_len);

void dsp_audio_start();

void dsp_audio_stop();

int dsp_audio_available();

int dsp_audio_payload(char *buf, int buf_len);

#endif