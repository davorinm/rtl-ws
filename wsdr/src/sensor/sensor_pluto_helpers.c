#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <iio.h>

#include "sensor_pluto_helpers.h"
#include "../tools/helpers.h"

/* static scratch mem for strings */
static char tmpstr[64];

/* returns ad9361 phy device */
static struct iio_device *get_ad9361_phy(struct iio_context *ctx)
{
    struct iio_device *dev = iio_context_find_device(ctx, "ad9361-phy");
    if (dev == NULL)
    {
        DEBUG("No ad9361-phy found\n");
    }

    return dev;
}

/* helper function generating channel names */
static char *get_ch_name(const char *type, int id)
{
    snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
    return tmpstr;
}

/* finds AD9361 local oscillator IIO configuration channels */
static bool get_lo_rx_chan(struct iio_context *ctx, struct iio_channel **chn)
{
    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 0), true);
    return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
static bool get_phy_rx_chan(struct iio_context *ctx, int chid, struct iio_channel **chn)
{
    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), false);
    return *chn != NULL;
}

/* write attribute: long long int */
static void wr_ch_lli(struct iio_channel *chn, const char *attribute, long long value)
{
    int ret;
    long long cValue;

    ret = iio_channel_attr_read_longlong(chn, attribute, &cValue);
    if (ret < 0)
    {
        DEBUG("Error %d reading longlong value to channel \"%s\"\n\"%lli\"\nvalue may not be supported.\n", ret, attribute, value);
        return;
    }

    // if (value == cValue)
    // {
    //     DEBUG("Reading longlong value from channel \"%s\" value \"%lli\" is equal to \"%lli\"\n", attribute, cValue, value);
    //     return;
    // }

    ret = iio_channel_attr_write_longlong(chn, attribute, value);
    if (ret < 0)
    {
        DEBUG("Error %d longlong value number to channel \"%s\"\n\"%lli\"\nvalue may not be supported.\n", ret, attribute, value);
        return;
    }

    DEBUG("Sucess writing longlong value to channel \"%s\" \"%lli\"\n", attribute, value);
}

/* write attribute: string */
static void wr_ch_str(struct iio_channel *chn, const char *attribute, const char *value)
{
    char buf[1024];
    int ret;
    ret = iio_channel_attr_read(chn, attribute, buf, sizeof(buf));
    if (ret < 0)
    {
        DEBUG("Error %d reading string from channel \"%s\"\nvalue may not be supported.\n", ret, attribute);
        return;
    }

    // if (strcmp(value, buf) == 0)
    // {
    //     DEBUG("Reading string from channel \"%s\" value \"%s\" is equal to \"%s\"\n", attribute, strdup(buf), value);
    //     return;
    // }

    ret = iio_channel_attr_write(chn, attribute, value);
    if (ret < 0)
    {
        DEBUG("Error %d writing string to channel \"%s\"\n\"%s\"\nvalue may not be supported.\n", ret, attribute, value);
        return;
    }
    
    DEBUG("Sucess writing string to channel \"%s\" \"%s\"\n", attribute, value);
}

// ======

/* finds AD9361 streaming IIO devices */
bool get_ad9361_rx_stream_dev(struct iio_context *ctx, struct iio_device **dev)
{
    *dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
    return *dev != NULL;
}

static int save_to_ini_chn_cb(struct iio_channel *chn, const char *attr, const char *val, size_t len, void *d)
{
    UNUSED(chn);
    UNUSED(len);
    UNUSED(d);

    DEBUG("Reading: \"%s\" value \"%s\" \n", attr, val);
    return 0;
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_rx_stream_ch(struct iio_context *ctx, struct sensor_config *cfg, int chid)
{
    struct iio_channel *chn = NULL;

    // Configure phy and lo channels
    printf("* Acquiring AD9361 phy channel %d\n", chid);
    if (!get_phy_rx_chan(ctx, chid, &chn))
    {
        printf("* ERROR Acquiring AD9361 phy channel %d\n", chid);
        return false;
    }

    iio_channel_attr_read_all(chn, save_to_ini_chn_cb, NULL);

    wr_ch_str(chn, "rf_port_select", cfg->rfport);
    wr_ch_lli(chn, "rf_bandwidth", cfg->bw_hz);
    wr_ch_lli(chn, "filter_fir_en", 0);
    wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

    //"manual fast_attack slow_attack hybrid" 0=auto 1=manual
    if (cfg->gain_mode == 0) {
        wr_ch_str(chn, "gain_control_mode", "slow_attack");  
    } else if (cfg->gain_mode == 1) {
        wr_ch_str(chn, "gain_control_mode", "manual");  
    }
 
    wr_ch_lli(chn, "hardwaregain", cfg->gain);

    iio_channel_attr_read_all(chn, save_to_ini_chn_cb, NULL);

    // Configure LO channel
    printf("* Acquiring AD9361 RX lo channel\n");
    if (!get_lo_rx_chan(ctx, &chn))
    {
        printf("* ERROR Acquiring AD9361 RX lo channel\n");
        return false;
    }
    
    iio_channel_attr_read_all(chn, save_to_ini_chn_cb, NULL);
    
    wr_ch_lli(chn, "frequency", cfg->lo_hz);

    iio_channel_attr_read_all(chn, save_to_ini_chn_cb, NULL);

    return true;
}

/* finds AD9361 streaming IIO channels */
bool get_ad9361_rx_stream_ch(struct iio_device *dev, int chid, struct iio_channel **chn)
{
    *chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), false);
    if (!*chn)
    {
        *chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), false);
    }
    return *chn != NULL;
}
