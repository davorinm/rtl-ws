#include <string.h>
#include <stdio.h>

#include "ws_handler.h"
#include "../sensor/sensor.h"
#include "../spectrum.h"
#include "../tools/helpers.h"
#include "../settings.h"

#define SEND_BUFFER_SIZE 1024 * 32

#define FREQ_CMD "freq"
#define SAMPLE_RATE_CMD "bw"
#define SPECTRUM_GAIN_CMD "spectrumgain"
#define START_CMD "start"
#define STOP_CMD "stop"
#define START_AUDIO_CMD "start_audio"
#define STOP_AUDIO_CMD "stop_audio"

static char *spectrum_buffer = NULL;
static char *audio_buffer = NULL;
static char *data_buffer = NULL;

void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss)
{
    UNUSED(c);

    int f = 0;
    int bw = 0;
    int spectrum_gain = 0;
    char *in_buffer = NULL;

    struct mg_str command = wm->data;
    MG_INFO(("Got command: %.*s", (int)command.len, command.ptr));

    if (mg_strstr(command, mg_str(FREQ_CMD)))
    {
        f = atoi(&command.ptr[strlen(FREQ_CMD)]) * 1000;
        INFO("Trying to tune to %d Hz...\n", f);
        sensor_set_frequency(f);
    }
    else if (mg_strstr(command, mg_str(SAMPLE_RATE_CMD)))
    {
        bw = atoi(&in_buffer[strlen(SAMPLE_RATE_CMD)]) * 1000;
        INFO("Trying to set sample rate to %d Hz...\n", bw);
        sensor_set_sample_rate(bw);
    }
    else if (mg_strstr(command, mg_str(SPECTRUM_GAIN_CMD)))
    {
        spectrum_gain = atoi(&command.ptr[strlen(SPECTRUM_GAIN_CMD)]);
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
    nn = spectrum_payload(spectrum_buffer + n, SEND_BUFFER_SIZE - n, 0);

    // Send data
    nnn = mg_ws_send(c, spectrum_buffer, n + nn, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

static void ws_update_audio(struct mg_connection *c)
{
    
}

static void ws_update_client(struct mg_connection *c)
{
    int n = 0, nnn = 0;

    // Set meta data
    n = sprintf(data_buffer, "Tf %u;b %u;s %lf", sensor_get_freq(), sensor_get_sample_rate(), sensor_get_gain());

    DEBUG("ws_update_client %s\n", data_buffer);

    // Send data
    nnn = mg_ws_send(c, data_buffer, n, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

void ws_handler_data(struct mg_connection *c, struct per_session_data__rtl_ws *pss)
{
    if (pss->update_client == 1)
    {
        pss->update_client = 0;
        ws_update_client(c);
    }
    
    if (pss->send_data)
    {
        if (spectrum_available())
        {
            ws_update_spectrum(c);
        }
    }
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