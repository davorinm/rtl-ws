#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libwebsockets.h>

#include "log.h"
#include "rtl_sensor.h"

#include "http_handler.h"
#include "ws_handler.h"

#include "rf_decimator.h"

#include "audio_main.h"
#include "cbb_main.h"

#define PORT                    8090

static volatile int force_exit = 0;

static void sighandler(int sig)
{
    force_exit = 1;
}

int main(int argc, char **argv)
{
    int status = 0;
    struct lws_context *context = NULL;
    struct lws_context_creation_info info = { 0 };

    signal(SIGINT, sighandler);
    
    INFO("Initializing audio processing...\n");
    audio_init();
    
    INFO("Initializing complex baseband processing...\n");
    cbb_init(DECIMATED_TARGET_BW_HZ);

    rf_decimator_add_callback(cbb_rf_decimator(), audio_fm_demodulator);

    // Init web socket
    ws_init();

    struct lws_protocols protos[] = { 
        *get_http_protocol(), 
        *get_ws_protocol()
    };
    
    info.port = PORT;
    info.protocols = protos;
    info.extensions = NULL;

    info.gid = -1;
    info.uid = -1;

    INFO("Initializing websockets...\n");
    context = lws_create_context(&info);
    if (context == NULL)
    {
        ERROR("Websocket init failed\n");
        return -1;
    }

    while (status >= 0 && !force_exit)
    {
        status = lws_service(context, 10);
    }


    INFO("Closing web socket...\n");
    ws_deinit();
    
    INFO("Closing complex baseband processing...\n");
    cbb_close();
    
    INFO("Closing audio processing...\n");
    audio_close();
    
    INFO("Destroying libwebsocket context\n");
    lws_context_destroy(context);

    return 0;
}
