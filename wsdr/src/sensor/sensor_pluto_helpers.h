#ifndef SENSOR_HELPERS_H
#define SENSOR_HELPERS_H

#include <stdlib.h>

/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))


/* common RX and TX streaming params */
struct sensor_config {
	long long bw_hz; // Analog banwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	uint64_t lo_hz; // Local oscillator frequency in Hz
    long long gain_mode; // AGC mode - manual, slow_attack, fast_attack
    double gain;
	const char* rfport; // Port name
	long long buffer_size;
};

bool get_ad9361_rx_stream_dev(struct iio_context *ctx, struct iio_device **dev);

bool cfg_ad9361_rx_stream_ch(struct iio_context *ctx, struct sensor_config *cfg, int chid);

bool get_ad9361_rx_stream_ch(struct iio_device *dev, int chid, struct iio_channel **chn);

#endif
