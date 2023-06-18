#include "web_handler.h"

#include "ws_handler.h"
#include "../settings.h"
#include "../tools/log.h"
#include "../mongoose/mongoose.h"

static const char *s_listen_on = "localhost:8000";
static const char *s_web_root = "./resources";

// Mongoose event manager
struct mg_mgr mgr;

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
            struct mg_http_serve_opts opts = {.root_dir = s_web_root};
            mg_http_serve_dir(c, ev_data, &opts);
        }
    }
    else if (ev == MG_EV_WS_OPEN)
    {
        INFO("MG_EV_WS_OPEN\n");
        c->data[0] = 'W'; // Mark this connection as an established WS client

        struct per_session_data__rtl_ws *pss = calloc(1, sizeof(struct per_session_data__rtl_ws));
        c->fn_data = pss;
    }
    else if (ev == MG_EV_WS_MSG)
    {
        INFO("MG_EV_WS_MSG\n");

        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;


        printf("GOT ECHO REPLY: [%.*s]\n", (int) wm->data.len, wm->data.ptr);

        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);

        struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)fn_data;
        ws_handler_callback(c, wm, pss);
    }
    else if (ev == MG_EV_WS_CTL)
    {
        INFO("MG_EV_WS_CTL\n");
    }

    (void)fn_data;
}

static void timer_fn(void *arg)
{
    struct mg_mgr *mgr = (struct mg_mgr *)arg;

    // Get first connection
    if (mgr->conns == NULL) {
        INFO("No conns\n");
        return;
    }

    struct mg_connection *c = mgr->conns;


    if (c->fn_data == NULL) {
        INFO("No sessin data\n");
        return;
    }
    
    struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)c->fn_data;

    ws_handler_data(c, pss);


    // Broadcast "hi" message to all connected websocket clients.
    // Traverse over all connections
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
        // Send only to marked connections
        if (c->data[0] == 'W') {
            mg_ws_send(c, "hi", 2, WEBSOCKET_OP_TEXT);
        }
    }
}

void web_init()
{
    INFO("Initializing web server...\n");

    // Mongoose init
    mg_mgr_init(&mgr); // Initialise event manager  
    mg_log_set(MG_LL_DEBUG);  // Set log level

    printf("Starting WS listener on %s/websocket\n", s_listen_on);

    // Timer
    mg_timer_add(&mgr, 1000, MG_TIMER_REPEAT, timer_fn, &mgr);

    // Create HTTP listener
    mg_http_listen(&mgr, s_listen_on, fn, NULL);
}

void web_poll()
{
    // mongoose loop
    mg_mgr_poll(&mgr, 200);
}

void web_close()
{
    INFO("Closing web server...\n");
    mg_mgr_free(&mgr);
}