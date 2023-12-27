#include "web_handler.h"

#include <unistd.h>
#include <pthread.h>
#include "ws_handler.h"
#include "../settings.h"
#include "../tools/helpers.h"
#include "../mongoose/mongoose.h"

static const char *s_listen_on = "0.0.0.0:8000";
#define WEB_ROOT "resources";
#define WEB_ROOT_EMB "/resources";

// Mongoose event manager
struct mg_mgr mgr;

pthread_t worker_thread;

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
//   /rest - respond with JSON string {"result": 123}
//   any other URI serves static files from s_web_root
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        INFO("MG_EV_OPEN\n");
        // c->is_hexdumping = 1;
    }
    else if (ev == MG_EV_HTTP_MSG)
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
    else if (ev == MG_EV_WRITE)
    {
        int64_t t = *(int64_t *)ev_data;
        MG_DEBUG(("MG_EV_WRITE bytes_written: %lld", t));
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
        INFO("MG_EV_WS_MSG\n");

        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;

        printf("GOT CMD: [%.*s]\n", (int)wm->data.len, wm->data.ptr);

        struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)fn_data;
        ws_handler_callback(c, wm, pss);
    }
    else if (ev == MG_EV_WS_CTL)
    {
        INFO("MG_EV_WS_CTL\n");
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        printf("GOT CTRL: [%.*s]\n", (int)wm->data.len, wm->data.ptr);
    }

    (void)fn_data;
}

static int force_exit = 0;

static void *timer_fn(void *arg)
{
    while (!force_exit)
    {
        usleep(100000);

        // INFO("W\n");
        struct mg_mgr *mgr = (struct mg_mgr *)arg;

        // Get first connection
        if (mgr->conns == NULL)
        {
            INFO("No conns\n");
            continue;
        }

        struct mg_connection *c = mgr->conns;

        if (c->fn_data == NULL)
        {
            // INFO("No sessin data\n");
            continue;
        }

        // if (!c->is_writable) {
        //     INFO("Connection is writable %i\n", c->is_writable);
        //     continue;
        // }

        struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)c->fn_data;
        ws_handler_data(c, pss);
    }

    return 0;
}

void web_init()
{
    int err;

    INFO("Initializing web server...\n");
    ws_init();

    // Mongoose init
    mg_mgr_init(&mgr);       // Initialise event manager
    mg_log_set(MG_LL_DEBUG); // Set log level

    printf("Starting WS listener on %s/websocket\n", s_listen_on);

    // Create HTTP listener
    mg_http_listen(&mgr, s_listen_on, fn, NULL);

    // Worker
    err = pthread_create(&worker_thread, NULL, timer_fn, (void *)&mgr);
    if (err)
    {
        printf("An error occured: %d", err);
    }
}

void web_poll()
{
    // mongoose loop
    mg_mgr_poll(&mgr, 100);
}

void web_close()
{
    force_exit = 1;

    printf("Waiting for the thread to end...\n");
    pthread_join(worker_thread, NULL);
    printf("Thread ended.\n");

    INFO("Closing web server...\n");
    mg_mgr_free(&mgr);

    ws_deinit();
}