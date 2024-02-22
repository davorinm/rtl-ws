#include "web_handler.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "../sensor/sensor.h"
#include "../dsp/dsp.h"
#include "../settings.h"
#include "../tools/helpers.h"
#include "../tools/timer.h"
#include "../mongoose/mongoose.h"

// WS buffer size
#define WS_BUFFER_SIZE 1024 * 32
static char *ws_data_buffer = NULL;

// WS commands
#define FREQ_CMD "freq"
#define BAND_WIDTH_CMD "rfbw"
#define SAMPLE_RATE_CMD "samplerate"
#define RF_GAIN_MODE_CMD "gain_mode"
#define RF_GAIN_CMD "rfgain"
#define START_CMD "start"
#define STOP_CMD "stop"
#define START_AUDIO_CMD "start_audio"
#define STOP_AUDIO_CMD "stop_audio"
#define KILL_CMD "kill"

// Mongoose event manager
static struct mg_mgr mgr;

static void ws_update_client(struct mg_connection *c)
{
    int n = 0, nnn = 0;

    // Set meta data
    n = sprintf(ws_data_buffer, "Tf %u;b %u;s %u;g %lf;y %u", sensor_get_freq(), sensor_get_rf_band_width(), sensor_get_sample_rate(), sensor_get_gain(), sensor_get_buffer_size());

    DEBUG("ws_update_client %s\n", ws_data_buffer);

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

static void ws_update_spectrum(struct mg_connection *c)
{
    int n = 0, nn = 0, nnn = 0;

    struct timespec time;
    double time_spent;

    // Set meta data
    n = sprintf(ws_data_buffer, "S");

    timer_start(&time);

    // Write spectrum
    nn = dsp_spectrum_get_payload(ws_data_buffer + n, WS_BUFFER_SIZE - n);

    timer_end(&time, &time_spent);
    timer_log("PAYLOAD", time_spent);

    timer_start(&time);

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n + nn, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }

    timer_end(&time, &time_spent);
    timer_log("SENDING_WS", time_spent);
}

static void ws_update_audio(struct mg_connection *c)
{
    int n = 0, nn = 0, nnn = 0;

    // Set meta data
    n = sprintf(ws_data_buffer, "A");

    // Write audio
    nn = dsp_audio_payload(ws_data_buffer + n, WS_BUFFER_SIZE - n);

    // Check length
    if (n + nn <= 0)
    {
        return;
    }

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n + nn, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }

    INFO("Audio header size %d, audio size %d, audio buffer size %d, socet size %d\n", n, nn, n + nn, nnn);
}

static void ws_stats_data(struct mg_connection *c)
{
    int n = 0, nnn = 0;

    // Set meta data
    n = sprintf(ws_data_buffer, "Df %lf;b %lf;s %lf;g %lf", timer_avg("GATHERING"), timer_avg("PROCESSING"), timer_avg("PAYLOAD"), timer_avg("SENDING_WS"));

    DEBUG("ws_stats_data %s\n", ws_data_buffer);

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

static void ws_handler_data(struct mg_connection *c)
{
    if (c->fn_data == NULL)
    {
        // INFO("No sessin data\n");
        return;
    }

    struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)c->fn_data;

    if (pss->update_client == 1)
    {
        pss->update_client = 0;
        ws_update_client(c);
        return;
    }

    if (pss->send_data)
    {
        if (dsp_spectrum_available())
        {
            ws_update_spectrum(c);
            return;
        }

        if (pss->audio_data == 1 && dsp_audio_available()) {
            ws_update_audio(c);
            return;
        }
    }
}

static void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss)
{
    UNUSED(c);

    int f = 0;
    int bw = 0;
    int sr = 0;
    int gain_mode = 0;
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
    else if (mg_strstr(command, mg_str(SAMPLE_RATE_CMD)))
    {
        sr = atoi(command.ptr + strlen(SAMPLE_RATE_CMD));
        INFO("Trying to set sample rate to %d Hz...\n", sr);
        sensor_set_sample_rate(sr);
    }
    else if (mg_strstr(command, mg_str(BAND_WIDTH_CMD)))
    {
        bw = atoi(command.ptr + strlen(BAND_WIDTH_CMD));
        INFO("Trying to set band width to %d Hz...\n", bw);
        sensor_set_rf_band_width(bw);
    }
    else if (mg_strstr(command, mg_str(RF_GAIN_MODE_CMD)))
    {
        gain_mode = atoi(command.ptr + strlen(RF_GAIN_MODE_CMD));
        INFO("Spectrum gain mode set to %i\n", gain_mode);
        sensor_set_gain_mode(gain_mode);
    }
    else if (mg_strstr(command, mg_str(RF_GAIN_CMD)))
    {
        spectrum_gain = atoi(command.ptr + strlen(RF_GAIN_CMD));
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
        dsp_audio_start();
        pss->audio_data = 1;
        INFO("Audio enabled\n");
    }
    else if (mg_strcmp(command, mg_str(STOP_AUDIO_CMD)) == 0)
    {
        dsp_audio_stop();
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

void web_socket_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_WS_OPEN)
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
}

static void web_page_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    UNUSED(fn_data);

    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, "/websocket"))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
        }
        else
        {
            // Serve static files
            struct mg_http_serve_opts opts;
            memset(&opts, 0, sizeof(opts));

#if MG_ENABLE_PACKED_FS
            // On embedded, use packed files
            opts.root_dir = WEB_ROOT_EMB;
            opts.fs = &mg_fs_packed;
#else
            // On workstations, use filesystem
            opts.root_dir = WEB_ROOT;
#endif
            mg_http_serve_dir(c, ev_data, &opts);
        }
    }
}

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (c->is_websocket)
    {
        web_socket_handler(c, ev, ev_data, fn_data);
    }
    else
    {
        web_page_handler(c, ev, ev_data, fn_data);
    }
}

static void ws_timer_fn(void *arg)
{
    struct mg_mgr *mgr = (struct mg_mgr *)arg;
    // Broadcast "hi" message to all connected websocket clients.
    // Traverse over all connections
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
        // Send only to marked connections
        if (c->data[0] != 'W')
        {
            continue;
        }

        ws_handler_data(c);
    }
}

static void ws_stats_fn(void *arg)
{
    struct mg_mgr *mgr = (struct mg_mgr *)arg;
    // Broadcast "hi" message to all connected websocket clients.
    // Traverse over all connections
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
        // Send only to marked connections
        if (c->data[0] != 'W')
        {
            continue;
        }

        ws_stats_data(c);
    }
}

void web_init()
{
    INFO("Initializing web server...\n");

    // Buffer
    ws_data_buffer = calloc(WS_BUFFER_SIZE, 1);

    // Mongoose init
    mg_mgr_init(&mgr); // Initialise event manager
    // mg_log_set(MG_LL_DEBUG); // Set log level

    INFO("Starting WS listener on %s/websocket\n", WEB_ADDRESS);

    // Timer worker for ws
    mg_timer_add(&mgr, 100, MG_TIMER_REPEAT, ws_timer_fn, &mgr);
    mg_timer_add(&mgr, 1000 * 60, MG_TIMER_REPEAT, ws_stats_fn, &mgr);

    // Create HTTP listener
    mg_http_listen(&mgr, WEB_ADDRESS, fn, NULL);
}

void web_poll()
{
    // mongoose loop, delay in ms if nothing to do
    mg_mgr_poll(&mgr, 10);
}

void web_close()
{
    INFO("Closing web server...\n");
    mg_mgr_free(&mgr);

    INFO("Releasing buffers...\n");
    free(ws_data_buffer);
}