#include <string.h>
#include <stdio.h>

#include "web_socket_handler.h"
#include "web_handler.h"
#include "../sensor/sensor.h"
#include "../dsp/spectrum.h"
#include "../tools/helpers.h"
#include "../settings.h"

#define SEND_BUFFER_SIZE 1024 * 64

#define FREQ_CMD "freq"
#define BAND_WIDTH_CMD "bw"
#define SAMPLE_RATE_CMD "samplerate"
#define SPECTRUM_GAIN_CMD "spectrumgain"
#define START_CMD "start"
#define STOP_CMD "stop"
#define START_AUDIO_CMD "start_audio"
#define STOP_AUDIO_CMD "stop_audio"
#define KILL_CMD "kill"

static char *spectrum_buffer = NULL;
static char *audio_buffer = NULL;
static char *data_buffer = NULL;

void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss)
{
    UNUSED(c);

    int f = 0;
    int bw = 0;
    int sr = 0;
    int spectrum_gain = 0;
    char *in_buffer = NULL;

    struct mg_str command = wm->data;
    MG_INFO(("Got command: %.*s", (int)command.len, command.ptr));

    if (mg_strstr(command, mg_str(FREQ_CMD)))
    {
        f = atoi(command.ptr + strlen(FREQ_CMD));
        INFO("Trying to tune to %d Hz...\n", f);
        sensor_set_frequency(f);
    }
    else if (mg_strstr(command, mg_str(BAND_WIDTH_CMD)))
    {
        bw = atoi(command.ptr + strlen(BAND_WIDTH_CMD));
        INFO("Trying to set band width to %d Hz...\n", bw);
        sensor_set_band_width(bw);
    }
    else if (mg_strstr(command, mg_str(SAMPLE_RATE_CMD)))
    {
        sr = atoi(command.ptr + strlen(SAMPLE_RATE_CMD));
        INFO("Trying to set sample rate to %d Hz...\n", sr);
        sensor_set_sample_rate(sr);
    }
    else if (mg_strstr(command, mg_str(SPECTRUM_GAIN_CMD)))
    {
        spectrum_gain = atoi(command.ptr + strlen(SPECTRUM_GAIN_CMD));
        INFO("Spectrum gain set to %d dB\n", spectrum_gain);
        sensor_set_gain(spectrum_gain);
    }
    else if (mg_strcmp(command, mg_str(START_CMD)) == 0)
    {
        pss->send_data = 1;
        INFO("START\n");
        sensor_start();
    }
    else if (mg_strcmp(command, mg_str(STOP_CMD)) == 0)
    {
        pss->send_data = 0;
        INFO("STOP\n");
        sensor_stop();
    }
    else if (mg_strcmp(command, mg_str(START_AUDIO_CMD)) == 0)
    {
        pss->audio_data = 1;
        INFO("Audio enabled\n");
    }
    else if (mg_strcmp(command, mg_str(STOP_AUDIO_CMD)) == 0)
    {
        pss->audio_data = 0;
        INFO("Audio disabled\n");
    }
    else if (mg_strcmp(command, mg_str(KILL_CMD)) == 0)
    {
        INFO("Killing process..\n");
        raise(SIGINT);
    }
    else
    {
        INFO("Unknown command!!!!\n");
    }

    free(in_buffer);

    // Something did change, update client
    pss->update_client = 1;
}

static void ws_update_spectrum(struct mg_connection *c)
{
    int n = 0, nn = 0, nnn = 0;

    // Set meta data
    n = sprintf(spectrum_buffer, "S");

    // Write spectrum
    nn = spectrum_payload(spectrum_buffer + n, SEND_BUFFER_SIZE - n);

    // Send data
    nnn = mg_ws_send(c, spectrum_buffer, n + nn, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }

    // DEBUG("spectrum bytes_written: %i, %i, %i \n", n, nn, nnn);
}

static void ws_update_client(struct mg_connection *c)
{
    int n = 0, nnn = 0;

    // Set meta data
    n = sprintf(data_buffer, "Tf %u;b %u;s %u;g %lf;y %u", sensor_get_freq(), sensor_get_band_width(), sensor_get_sample_rate(), sensor_get_gain(), sensor_get_buffer_size());

    DEBUG("ws_update_client %s\n", data_buffer);

    // Send data
    nnn = mg_ws_send(c, data_buffer, n, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

void ws_handler_data(struct mg_connection *c)
{
    DEBUG("ws_handler_data\n");

    if (c->fn_data == NULL)
    {
        // INFO("No sessin data\n");
        return;
    }

    struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)c->fn_data;

    if (pss->update_client == 1)
    {
        DEBUG("update_client\n");
        pss->update_client = 0;
        ws_update_client(c);
        return;
    }

    if (pss->send_data && pss->writable)
    {
        if (spectrum_available())
        {
            ws_update_spectrum(c);
            return;
        }
        else
        {

            DEBUG("spectrum not available\n");
        }
    }
    else
    {
        DEBUG("send_data = 0\n");
    }
}

void web_socket_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        INFO("MG_EV_OPEN\n");
        // c->is_hexdumping = 1;
    }
    else if (ev == MG_EV_WRITE)
    {
        long *bytes_written = (long *)ev_data;
        DEBUG("MG_EV_WRITE bytes_written: %ld, is_writable %i, is_websocket %i\n", *bytes_written, c->is_writable, c->is_websocket);

        if (c->data[0] == 'W' && c->is_websocket && !c->is_closing)
        {
            struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)fn_data;
            pss->writable = c->is_writable;

            INFO("Connection is_writable %i, is_websocket %i\n", c->is_writable, c->is_websocket);
            ws_handler_data(c);
        }

        // INFO("Connection is_writable %i\n", c->is_writable);
        // INFO("Connection is_websocket %i\n", c->is_websocket);

        // if (c->is_writable)
        // {
        //     ws_handler_data(c);
        // }
    }
    else if (ev == MG_EV_WS_OPEN)
    {
        INFO("MG_EV_WS_OPEN\n");
        c->data[0] = 'W'; // Mark this connection as an established WS client

        struct per_session_data__rtl_ws *pss = calloc(1, sizeof(struct per_session_data__rtl_ws));
        pss->update_client = 1;
        c->fn_data = pss;
    }
    else if (ev == MG_EV_WS_MSG)
    {
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        DEBUG("MG_EV_WS_MSG: [%.*s] falgs: %d\n", (int)wm->data.len, wm->data.ptr, wm->flags);

        struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)fn_data;
        ws_handler_callback(c, wm, pss);
    }
    else if (ev == MG_EV_WS_CTL)
    {
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        DEBUG("MG_EV_WS_CTL: [%.*s] falgs: %d\n", (int)wm->data.len, wm->data.ptr, wm->flags);
    }
    else if (ev == MG_EV_CLOSE)
    {
        DEBUG("MG_EV_CLOSE is_websocket %i %c\n", c->is_websocket, c->data[0]);
    }

    (void)fn_data;
}

void ws_deinit()
{
    free(spectrum_buffer);
    free(audio_buffer);
    free(data_buffer);
}

void ws_init()
{
    spectrum_buffer = calloc(SEND_BUFFER_SIZE, 1);
    audio_buffer = calloc(SEND_BUFFER_SIZE, 1);
    data_buffer = calloc(SEND_BUFFER_SIZE, 1);
}