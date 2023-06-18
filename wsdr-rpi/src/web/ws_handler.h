#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include "../mongoose/mongoose.h"

struct per_session_data__rtl_ws {
    int id;
    int send_data;
    int audio_data;
    int sent_audio_fragments;
};

void ws_handler_data(struct mg_connection *c, struct per_session_data__rtl_ws *pss);

void ws_handler_callback(struct mg_connection *c, struct mg_ws_message *wm, struct per_session_data__rtl_ws *pss);

void ws_deinit();

void ws_init();

#endif
