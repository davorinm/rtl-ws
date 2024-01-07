#include <string.h>
#include <stdio.h>

#include "web_page_handler.h"
#include "web_handler.h"
#include "../tools/helpers.h"

void web_page_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
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
    else if (ev == MG_EV_CLOSE)
    {
        DEBUG("MG_EV_CLOSE is_websocket %i %c\n", c->is_websocket, c->data[0]);
    }

    (void)fn_data;
}
