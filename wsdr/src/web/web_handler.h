#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#define WEB_ADDRESS "0.0.0.0:8000"
#define WEB_ROOT "resources"
#define WEB_ROOT_EMB "/resources"

struct per_session_data__rtl_ws {
    int id;
    int send_data;
    int audio_data;
    int update_client;
    int writable;
};

void web_init();

void web_poll();

void web_close();

#endif