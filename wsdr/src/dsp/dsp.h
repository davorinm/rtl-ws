#ifndef DSP_H
#define DSP_H

void dsp_init();

int dsp_spectrum_available();

int dsp_spectrum_payload(char *buf, int buf_len);

void dsp_audio_start();

void dsp_audio_stop();

int dsp_audio_available();

int dsp_audio_payload(char *buf, int buf_len);

void dsp_close();

#endif