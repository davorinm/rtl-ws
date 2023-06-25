#include <string.h>
#include <stdio.h>

#include "ws_handler.h"
#include "../dsp/cbb_main.h"
#include "../audio_main.h"
#include "../tools/log.h"
#include "../settings.h"

#define SEND_BUFFER_SIZE 1024 * 64

#define FREQ_CMD "freq"
#define SAMPLE_RATE_CMD "bw"
#define SPECTRUM_GAIN_CMD "spectrumgain"
#define START_CMD "start"
#define STOP_CMD "stop"
#define START_AUDIO_CMD "start_audio"
#define STOP_AUDIO_CMD "stop_audio"

static int current_user_id = 0;
static int latest_user_id = 1;

static volatile int spectrum_gain = 0;

static char *send_buffer = NULL;

void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss)
{
    int f = 0;
    int bw = 0;
    char *in_buffer = NULL;
    struct rtl_dev *dev = cbb_get_rtl_dev();

    struct mg_str command = wm->data;
    MG_INFO(("Got command: %.*s", (int)command.len, command.ptr));

    if (mg_strstr(command, mg_str(FREQ_CMD))) // strncmp(s1->ptr
    {
        f = atoi(&command.ptr[strlen(FREQ_CMD)]) * 1000;
        INFO("Trying to tune to %d Hz...\n", f);
        rtl_set_frequency(dev, f);
    }
    else if (mg_strcmp(command, mg_str(SAMPLE_RATE_CMD)) == 0)
    {
        bw = atoi(&in_buffer[strlen(SAMPLE_RATE_CMD)]) * 1000;
        INFO("Trying to set sample rate to %d Hz...\n", bw);
        rtl_set_sample_rate(dev, bw);
        rf_decimator_set_parameters(cbb_rf_decimator(), rtl_sample_rate(dev), rtl_sample_rate(dev) / DECIMATED_TARGET_BW_HZ);
    }
    else if (mg_strcmp(command, mg_str(SPECTRUM_GAIN_CMD)) == 0)
    {
        spectrum_gain = atoi(&in_buffer[strlen(SPECTRUM_GAIN_CMD)]);
        INFO("Spectrum gain set to %d dB\n", spectrum_gain);
        // rtl_set_gain();
    }
    else if (mg_strcmp(command, mg_str(START_CMD)) == 0)
    {
        pss->send_data = 1;
        INFO("START\n");
    }
    else if (mg_strcmp(command, mg_str(STOP_CMD)) == 0)
    {
        pss->send_data = 0;
        INFO("STOP\n");
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
}

void ws_handler_data(struct mg_connection *c, struct per_session_data__rtl_ws *pss)
{
    int n = 0, nn = 0, nnn = 0;
    char tmpbuffer[30] = {0};
    struct rtl_dev *dev = cbb_get_rtl_dev();

    if (pss->send_data)
    {
        if (cbb_new_spectrum_available())
        {
            // Clear buffer
            memset(send_buffer, 0, SEND_BUFFER_SIZE);

            // Set meta data
            // n = sprintf(send_buffer, "Sf%u;b %u;s %d;d", rtl_freq(dev), rtl_sample_rate(dev), spectrum_gain);
            n = sprintf(send_buffer, "S");

            // Write spectrum
            nn = cbb_get_spectrum_payload(send_buffer + n, SEND_BUFFER_SIZE - n, spectrum_gain);

            // Send data
            nnn = mg_ws_send(c, send_buffer, n + nn, WEBSOCKET_OP_BINARY);
        }

        if (pss->audio_data == 1 && audio_new_audio_available())
        {
            // Clear buffer
            memset(send_buffer, 0, SEND_BUFFER_SIZE);

            // Set meta data
            n = sprintf(send_buffer, "A   ");

            // Write audio
            nn = audio_get_audio_payload(send_buffer + n, SEND_BUFFER_SIZE - n);

            // Send data
            nnn = mg_ws_send(c, send_buffer, n + nn, WEBSOCKET_OP_BINARY);

            INFO("Audio header size %d, audio size %d, audio buffer size %d, socet size %d\n", n, nn, n + nn, nnn);
        }
    }

    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

void ws_deinit()
{
    free(send_buffer);
}

void ws_init()
{
    send_buffer = calloc(SEND_BUFFER_SIZE, 1);
}