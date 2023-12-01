#ifndef RATE_LOGGER_H
#define RATE_LOGGER_H

typedef struct rate_logger rate_logger;

rate_logger* rate_logger_alloc();

void rate_logger_set_parameters(rate_logger* r, const char* logger_name, int log_rate_ms);

void rate_logger_log(rate_logger* r, int sample_count);

void rate_logger_free(rate_logger* r);

#endif