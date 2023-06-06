#include <libwebsockets.h>

#include "web_handler.h"
#include "http_handler.h"
#include "ws_handler.h"
#include "../settings.h"
#include "../tools/log.h"

#include "mongoose.h"

static const char *s_listen_on = "ws://localhost:8000";
static const char *s_web_root = ".";

// Mongoose event manager
struct mg_mgr mgr;

static struct lws_context *context = NULL;
static struct lws_context_creation_info info = {0};

static struct lws_protocols protos[] = {
    {"http-only", http_handler_callback, sizeof(struct per_session_data__http)},
    {"rtl-ws-protocol", ws_handler_callback, sizeof(struct per_session_data__rtl_ws)}};

static int status = 0;

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
//   /rest - respond with JSON string {"result": 123}
//   any other URI serves static files from s_web_root
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        // c->is_hexdumping = 1;
    }
    else if (ev == MG_EV_WS_OPEN)
    {
        c->data[0] = 'W'; // Mark this connection as an established WS client
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
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
        mg_iobuf_del(&c->recv, 0, c->recv.len);
    }

    (void)fn_data;
}

// Push to all watchers
static void push(struct mg_mgr *mgr, const char *name, const void *data)
{
    struct mg_connection *c;
    for (c = mgr->conns; c != NULL; c = c->next)
    {
        if (c->data[0] != 'W')
        {
            continue;
        }
        mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m:%m,%m:%m}", MG_ESC("name"), MG_ESC(name), MG_ESC("data"), MG_ESC(data));
    }
}

void web_init()
{
    // mg_mgr_init(&mgr); // Initialise event manager
    // printf("Starting WS listener on %s/websocket\n", s_listen_on);
    // mg_http_listen(&mgr, s_listen_on, fn, NULL); // Create HTTP listener

    // Init web socket
    ws_init();

    info.port = PORT;
    info.protocols = protos;
    info.extensions = NULL;

    info.gid = -1;
    info.uid = -1;

    INFO("Initializing websockets...\n");
    context = lws_create_context(&info);
    if (context == NULL)
    {
        ERROR("Websocket init failed\n");
        // return -1;
    }
}

void web_poll()
{
    // mongoose loop
    // mg_mgr_poll(&mgr, 1000);

    status = lws_service(context, 10);
    if (status < 0)
    {
        ERROR("web_pool lws_service failed\n");
    }
}

void web_close()
{
    // mg_mgr_free(&mgr);

    INFO("Closing web socket...\n");
    ws_deinit();

    INFO("Destroying libwebsocket context\n");
    lws_context_destroy(context);
}