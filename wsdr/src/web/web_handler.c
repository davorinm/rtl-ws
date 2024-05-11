#include "web_handler.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "../dsp/dsp.h"
#include "../tools/timer.h"
#include "../tools/helpers.h"
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
#define FILTER_FREQ_CMD "filter_freq"
#define FILTER_FREQ_WIDTH_CMD "filter_freq_width"
#define KILL_CMD "kill"

// Mongoose event manager
static struct mg_mgr mgr;

static void ws_update_client(struct mg_connection *c)
{
    int n = 0, nnn = 0;

    // Set data
    n = sprintf(ws_data_buffer, "T{\"params\":{\"freq\":%llu,\"bw\": %u,\"sr\": %u,\"gain\": %lf,\"bs\": %u,\"ff\": %u,\"fw\": %u},\"time\":{\"gath\":%lf,\"proc\":%lf,\"pay\":%lf,\"ws\":%lf,\"aud\":%lf,\"spect\":%lf}}",
                dsp_sensor_get_freq(),
                dsp_sensor_get_rf_band_width(),
                dsp_sensor_get_sample_rate(),
                dsp_sensor_get_gain(),
                dsp_sensor_get_buffer_size(),
                dsp_filter_get_freq(),
                dsp_filter_get_width(),
                timer_avg("GATHERING"), 
                timer_avg("PROCESSING"), 
                timer_avg("PAYLOAD"), 
                timer_avg("SENDING_WS"), 
                timer_avg("AUDIO"), 
                timer_avg("SPECTRUM"));

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

    // Set data
    n = sprintf(ws_data_buffer, "S");

    // Write spectrum
    nn = dsp_spectrum_payload(ws_data_buffer + n, WS_BUFFER_SIZE - n);

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n + nn, WEBSOCKET_OP_BINARY);
    if (nnn < 0)
    {
        ERROR("Writing failed, error code == %d\n", nnn);
    }
}

static void ws_update_audio(struct mg_connection *c)
{
    int n = 0, nn = 0, nnn = 0;

    // Set data
    n = sprintf(ws_data_buffer, "A   ");

    // Write audio
    nn = dsp_audio_payload(ws_data_buffer + n, WS_BUFFER_SIZE - n);

    // Send data
    nnn = mg_ws_send(c, ws_data_buffer, n + nn, WEBSOCKET_OP_BINARY);
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
    }

    if (pss->send_data)
    {
        if (dsp_spectrum_available())
        {
            ws_update_spectrum(c);
        }

        if (pss->audio_data == 1 && dsp_audio_available())
        {
            ws_update_audio(c);
        }
    }
}

static bool str_prefix(const struct mg_str pre, const char *str)
{
    return strncmp(pre.ptr, str, strlen(str)) == 0;
}

static void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss)
{
    UNUSED(c);

    uint64_t f = 0;
    int bw = 0;
    int sr = 0;
    int gain_mode = 0;
    int spectrum_gain = 0;
    int filter_freq = 0;
    int filter_freq_width = 0;
    char *in_buffer = NULL;

    struct mg_str command = wm->data;
    MG_INFO(("Got command: %.*s", (int)command.len, command.ptr));

    if (str_prefix(command, FREQ_CMD))
    {
        f = atoll(command.ptr + strlen(FREQ_CMD));
        INFO("Trying to tune to %llu Hz...\n", f);
        dsp_sensor_set_frequency(f);
    }
    else if (str_prefix(command, SAMPLE_RATE_CMD))
    {
        sr = atoi(command.ptr + strlen(SAMPLE_RATE_CMD));
        INFO("Trying to set sample rate to %d Hz...\n", sr);
        dsp_sensor_set_sample_rate(sr);
    }
    else if (str_prefix(command, BAND_WIDTH_CMD))
    {
        bw = atoi(command.ptr + strlen(BAND_WIDTH_CMD));
        INFO("Trying to set band width to %d Hz...\n", bw);
        dsp_sensor_set_rf_band_width(bw);
    }
    else if (str_prefix(command, RF_GAIN_MODE_CMD))
    {
        gain_mode = atoi(command.ptr + strlen(RF_GAIN_MODE_CMD));
        INFO("Spectrum gain mode set to %i\n", gain_mode);
        dsp_sensor_set_gain_mode(gain_mode);
    }
    else if (str_prefix(command, RF_GAIN_CMD))
    {
        spectrum_gain = atoi(command.ptr + strlen(RF_GAIN_CMD));
        INFO("Spectrum gain set to %d dB\n", spectrum_gain);
        dsp_sensor_set_gain(spectrum_gain);
    }
    else if (str_prefix(command, START_CMD))
    {
        pss->send_data = 1;
        INFO("START\n");
        dsp_sensor_start();
    }
    else if (str_prefix(command, STOP_CMD))
    {
        pss->send_data = 0;
        INFO("STOP\n");
        dsp_sensor_stop();
    }
    else if (str_prefix(command, START_AUDIO_CMD))
    {
        dsp_audio_start();
        pss->audio_data = 1;
        INFO("Audio enabled\n");
    }
    else if (str_prefix(command, STOP_AUDIO_CMD))
    {
        dsp_audio_stop();
        pss->audio_data = 0;
        INFO("Audio disabled\n");
    }
    else if (str_prefix(command, KILL_CMD))
    {
        INFO("Killing process..\n");
        raise(SIGINT);
    }
    else if (str_prefix(command, FILTER_FREQ_CMD))
    {
        filter_freq = atoi(command.ptr + strlen(FILTER_FREQ_CMD));
        INFO("Filter frequency set to %d Hz\n", filter_freq);
        dsp_filter_freq(filter_freq);
    }
    else if (str_prefix(command, FILTER_FREQ_WIDTH_CMD))
    {
        filter_freq_width = atoi(command.ptr + strlen(FILTER_FREQ_WIDTH_CMD));
        INFO("Filter frequency width set to %d Hz\n", filter_freq_width);
        dsp_filter_width(filter_freq_width);
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

        ws_update_client(c);
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
    mg_timer_add(&mgr, 1000 * 10, MG_TIMER_REPEAT, ws_stats_fn, &mgr);

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