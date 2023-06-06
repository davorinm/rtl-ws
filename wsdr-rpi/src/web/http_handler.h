#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <libwebsockets.h>

struct per_session_data__http
{
    int fd;
};

int http_handler_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

#endif
