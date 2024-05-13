#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "dsp_common.h"

void spectrum_init(unsigned int sensor_count);

void spectrum_data(const cmplx_dbl *signal, unsigned int len);

int spectrum_available();

int spectrum_payload(char *buf, unsigned int buf_len);

void spectrum_close();

#endif
