#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include <libwebsockets.h>

#define DECIMATED_TARGET_BW_HZ  192000

struct lws_protocols* get_ws_protocol();

void ws_deinit();

void ws_init();

#endif
