#include "web_handler.h"

#include <unistd.h>
#include <pthread.h>
#include "web_page_handler.h"
#include "web_socket_handler.h"
#include "../settings.h"
#include "../tools/helpers.h"
#include "../mongoose/mongoose.h"

// Mongoose event manager
struct mg_mgr mgr;

pthread_t worker_thread;

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
//   /rest - respond with JSON string {"result": 123}
//   any other URI serves static files from s_web_root
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (c->is_websocket) {
        web_socket_handler(c, ev, ev_data, fn_data);
    } else {
        web_page_handler(c, ev, ev_data, fn_data);
    }
}

static void timer_fn(void *arg)
{
    DEBUG("timer_fn\n");

    struct mg_mgr *mgr = (struct mg_mgr *)arg;
    // Broadcast "hi" message to all connected websocket clients.
    // Traverse over all connections
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
        // Send only to marked connections
        if (c->is_websocket)
        {
            long bytes_written = 0;
            mg_call(c, MG_EV_WRITE, &bytes_written);
        }
    }
}

void web_init()
{
    // int err;

    INFO("Initializing web server...\n");
    ws_init();

    // Mongoose init
    mg_mgr_init(&mgr); // Initialise event manager
    // mg_log_set(MG_LL_DEBUG); // Set log level

    printf("Starting WS listener on %s/websocket\n", WEB_ADDRESS);

    // Timer worker for ws
    mg_timer_add(&mgr, 100, MG_TIMER_REPEAT, timer_fn, &mgr);

    // Create HTTP listener
    mg_http_listen(&mgr, WEB_ADDRESS, fn, NULL);
}

void web_poll()
{
    // mongoose loop
    mg_mgr_poll(&mgr, 100);
}

void web_close()
{
    printf("Waiting for the thread to end...\n");
    pthread_join(worker_thread, NULL);
    printf("Thread ended.\n");

    INFO("Closing web server...\n");
    mg_mgr_free(&mgr);

    ws_deinit();
}