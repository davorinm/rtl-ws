#ifndef WEB_PAGE_HANDLER_H
#define WEB_PAGE_HANDLER_H

#include "../mongoose/mongoose.h"

void web_page_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

#endif
