#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include <libwebsockets.h>

struct lws_protocols* get_ws_protocol();

void ws_deinit();

void ws_init();

#endif
