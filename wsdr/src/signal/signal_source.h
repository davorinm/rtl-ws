#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H

#include "../dsp/dsp_common.h"

typedef void (*signal_source_callback)(const cmplx_u8 *, int);

uint32_t signal_get_sample_rate();

int signal_set_sample_rate(uint32_t fs);

uint32_t signal_get_freq();

int signal_set_frequency(uint32_t f);

void signal_source_init();

void signal_source_start();

void signal_source_add_callback(signal_source_callback callback);

void signal_source_remove_callbacks();

void signal_source_stop();

void signal_source_close();

#endif
