#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <stdio.h>

#define RTL_WS_LOG_PREFIX(out) fprintf(out, __FILE__); fprintf(out, ":"); fprintf(out, "%d", __LINE__);

#ifdef RTL_WS_DEBUG
#define DEBUG(...) RTL_WS_LOG_PREFIX(stdout); fprintf(stdout, " [D] "); fprintf(stdout, __VA_ARGS__); fflush(stdout);
#else
#define DEBUG(...)
#endif

#define INFO(...) RTL_WS_LOG_PREFIX(stdout); fprintf(stdout, " [I] "); fprintf(stdout, __VA_ARGS__); fflush(stdout);

#define ERROR(...) RTL_WS_LOG_PREFIX(stderr); fprintf(stderr, " [E] "); fprintf(stderr, __VA_ARGS__); fflush(stderr);

#define UNUSED(x) (void)(x)

uint64_t timestamp();

#endif
