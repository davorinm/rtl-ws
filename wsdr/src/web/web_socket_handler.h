#ifndef WEB_SOCKET_HANDLER_H
#define WEB_SOCKET_HANDLER_H

#include "../mongoose/mongoose.h"

void web_socket_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

void ws_deinit();

void ws_init();

#endif
