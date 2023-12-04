#ifdef SENSOR_PLUTO

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <iio.h>

#include "../tools/helpers.h"

struct pluto_dev
{
	struct iio_device *rx;
	uint32_t f;
	uint32_t fs;
	double gain;
};

/* helper macros */
#define MHZ(x) ((long long)(x * 1000000.0 + .5))
#define GHZ(x) ((long long)(x * 1000000000.0 + .5))

/* RX is input, TX is output */
enum iodev
{
	RX,
	TX
};

/* common RX and TX streaming params */
struct stream_cfg
{
	long long bw_hz;	// Analog banwidth in Hz
	long long fs_hz;	// Baseband sample rate in Hz
	long long lo_hz;	// Local oscillator frequency in Hz
	const char *rfport; // Port name
};

/* static scratch mem for strings */
static char tmpstr[64];

/* IIO structs required for streaming */
static struct iio_context *ctx = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_buffer *rxbuf = NULL;

// Streaming devices
struct iio_device *rx;

// RX and TX sample counters
size_t nrx = 0;

/* write attribute: long long int */
static void wr_ch_lli(struct iio_channel *chn, const char *what, long long val)
{
	int v = 0;
	v = iio_channel_attr_write_longlong(chn, what, val);
	if (v < 0)
	{
		DEBUG("Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what);
	}
}

/* write attribute: string */
static void wr_ch_str(struct iio_channel *chn, const char *what, const char *str)
{
	int v = 0;
	v = iio_channel_attr_write(chn, what, str);
	if (v < 0)
	{
		DEBUG("Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what);
	}
}

/* helper function generating channel names */
static char *get_ch_name(const char *type, int id)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
	return tmpstr;
}

/* returns ad9361 phy device */
static struct iio_device *get_ad9361_phy(void)
{
	struct iio_device *dev = iio_context_find_device(ctx, "ad9361-phy");
	if (dev == NULL)
	{
		DEBUG("Pluto: get_ad9361_phy ERROR\n");
		return NULL;
	}

	return dev;
}

/* finds AD9361 streaming IIO devices */
static bool get_ad9361_stream_dev(enum iodev d, struct iio_device **dev)
{
	switch (d)
	{
	case RX:
		*dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
		return *dev != NULL;
	default:
		return false;
	}
}

/* finds AD9361 streaming IIO channels */
static bool get_ad9361_stream_ch(enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
{
	*chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
	if (!*chn)
	{
		*chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
	}
	return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
static bool get_phy_chan(enum iodev d, int chid, struct iio_channel **chn)
{
	switch (d)
	{
	case RX:
		*chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("voltage", chid), false);
		return *chn != NULL;
	default:
		return false;
	}
}

/* finds AD9361 local oscillator IIO configuration channels */
static bool get_lo_chan(enum iodev d, struct iio_channel **chn)
{
	switch (d)
	{
		// LO chan is always output, i.e. true
	case RX:
		*chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("altvoltage", 0), true);
		return *chn != NULL;
	default:
		return false;
	}
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch(struct stream_cfg *cfg, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;

	// Configure phy and lo channels
	printf("* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan(type, chid, &chn))
	{
		return false;
	}

	wr_ch_str(chn, "rf_port_select", cfg->rfport);
	wr_ch_lli(chn, "rf_bandwidth", cfg->bw_hz);
	wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

	// Configure LO channel
	printf("* Acquiring AD9361 %s lo channel\n", "RX");
	if (!get_lo_chan(type, &chn))
	{
		return false;
	}

	wr_ch_lli(chn, "frequency", cfg->lo_hz);

	return true;
}

int pluto_init()
{
	int r = 0;

	// Stream configurations
	struct stream_cfg rxcfg;

	// RX stream config
	rxcfg.bw_hz = MHZ(5);		 // 2 MHz rf bandwidth
	rxcfg.fs_hz = MHZ(10);		 // 2.5 MS/s rx sample rate
	rxcfg.lo_hz = MHZ(102.8);		 // 2.5 GHz rf frequency
	rxcfg.rfport = "A_BALANCED"; // port A (select for rf freq.)

	DEBUG("* Acquiring IIO context\n");
	ctx = iio_create_local_context();
	if (ctx == NULL)
	{
		DEBUG("Pluto: No context\n");
		return -1;
	}

	DEBUG("* Acquiring devices count\n");
	r = iio_context_get_devices_count(ctx);
	if (r < 0)
	{
		DEBUG("Pluto: No devices\n");
		return r;
	}

	DEBUG("* Acquiring AD9361 streaming devices\n");
	r = get_ad9361_stream_dev(RX, &rx);
	if (r < 0)
	{
		DEBUG("Pluto: No rx dev found\n");
		return r;
	}

	DEBUG("* Configuring AD9361 for streaming\n");
	r = cfg_ad9361_streaming_ch(&rxcfg, RX, 0);
	if (r < 0)
	{
		DEBUG("Pluto: RX port 0 not found\n");
		return r;
	}

	DEBUG("* Initializing AD9361 IIO streaming channels\n");
	r = get_ad9361_stream_ch(RX, rx, 0, &rx0_i);
	if (r < 0)
	{
		DEBUG("RX chan i not found\n");
		return r;
	}

	r = get_ad9361_stream_ch(RX, rx, 1, &rx0_q);
	if (r < 0)
	{
		DEBUG("TRX chan q not found\n");
		return r;
	}

	DEBUG("* Enabling IIO streaming channels\n");
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);

	DEBUG("* Creating non-cyclic IIO buffers with 1 MiS\n");
	rxbuf = iio_device_create_buffer(rx, 1024 * 1024, false);
	if (!rxbuf)
	{
		perror("Could not create RX buffer");
		return -3;
	}

	return 0;
}

int pluto_set_frequency(uint32_t f)
{
	UNUSED(f);

    int r = 0;
	return r;
}

int pluto_set_sample_rate(uint32_t fs)
{
	UNUSED(fs);

    int r = 0;
	return r;
}

int pluto_set_gain(double gain)
{
	UNUSED(gain);

    int r = 0;
	return r;
}

uint32_t pluto_freq()
{
	return MHZ(102.8);
}

uint32_t pluto_sample_rate()
{
	return MHZ(10);
}

double pluto_gain()
{
	return 0;
}

int pluto_read_async(void (*callback)(unsigned char *, uint32_t, void *), void *user)
{
	DEBUG("* Starting IO streaming (press CTRL+C to cancel)\n");

	ssize_t nbytes_rx;
	char *p_dat, *p_end;
	ptrdiff_t p_inc;

	// Refill RX buffer
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0)
	{
		printf("Error refilling buf %d\n", (int)nbytes_rx);
		return -2;
	}

	// // READ: Get pointers to RX buf and read IQ from RX buf port 0
	// p_inc = iio_buffer_step(rxbuf);
	// p_end = iio_buffer_end(rxbuf);
	// for (p_dat = (char *)iio_buffer_first(rxbuf, rx0_i); p_dat < p_end; p_dat += p_inc)
	// {
	// 	// Example: swap I and Q
	// 	const int16_t i = ((int16_t *)p_dat)[0]; // Real (I)
	// 	const int16_t q = ((int16_t *)p_dat)[1]; // Imag (Q)
	// 	((int16_t *)p_dat)[0] = q;
	// 	((int16_t *)p_dat)[1] = i;
	// }

	callback(rxbuf, nbytes_rx, user);

	// Sample counter increment and status output
	nrx += nbytes_rx / iio_device_get_sample_size(rx);
	printf("\tRX %8.2f MSmp\n", nrx / 1e6);

	return 0;
}

void pluto_cancel()
{
}

void pluto_close()
{
	printf("* Destroying buffers\n");
	if (rxbuf)
	{
		iio_buffer_destroy(rxbuf);
	}

	printf("* Disabling streaming channels\n");
	if (rx0_i)
	{
		iio_channel_disable(rx0_i);
	}
	if (rx0_q)
	{
		iio_channel_disable(rx0_q);
	}

	printf("* Destroying context\n");
	if (ctx)
	{
		iio_context_destroy(ctx);
	}
	exit(0);
}

#endif