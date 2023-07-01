#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "tools/log.h"
#include "web/web_handler.h"
#include "dsp/rf_decimator.h"
#include "audio_main.h"
#include "dsp/cbb_main.h"

static volatile int force_exit = 0;

static void sighandler(int sig)
{
    force_exit = 1;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sighandler);

    INFO("Initializing audio processing...\n");
    audio_init();

    INFO("Initializing complex baseband processing...\n");
    cbb_init();

    rf_decimator_add_callback(cbb_rf_decimator(), audio_fm_demodulator);

    INFO("Initializing web service...\n");
    web_init();

    // Loop
    while (!force_exit)
    {
        web_poll();
    }

    INFO("Closing web context\n");
    web_close();

    INFO("Closing complex baseband processing...\n");
    cbb_close();

    INFO("Closing audio processing...\n");
    audio_close();

    return 0;
}
