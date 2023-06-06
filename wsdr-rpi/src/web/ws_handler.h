#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include <libwebsockets.h>

struct per_session_data__rtl_ws {
    int id;
    int send_data;
    int audio_data;
    int sent_audio_fragments;
};

int ws_handler_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

void ws_deinit();

void ws_init();

#endif
