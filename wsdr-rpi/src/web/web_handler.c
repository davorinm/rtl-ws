#include <libwebsockets.h>

#include "web_handler.h"
#include "http_handler.h"
#include "ws_handler.h"
#include "../settings.h"
#include "../tools/log.h"

static struct lws_context *context = NULL;
static struct lws_context_creation_info info = {0};

static struct lws_protocols protos[] = {
    {"http-only", http_handler_callback, sizeof(struct per_session_data__http)},
    {"rtl-ws-protocol", ws_handler_callback, sizeof(struct per_session_data__rtl_ws)}};

static int status = 0;

void web_init()
{
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
    status = lws_service(context, 10);
    if (status < 0)
    {
        ERROR("web_pool lws_service failed\n");
    }
}

void web_close()
{
    INFO("Closing web socket...\n");
    ws_deinit();

    INFO("Destroying libwebsocket context\n");
    lws_context_destroy(context);
}