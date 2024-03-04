#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "dsp_common.h"

void spectrum_init();

void spectrum_process(const cmplx_s32 *signal, int len);

int spectrum_available();

int spectrum_payload(char *buf, int buf_len);

void spectrum_close();

#endif
