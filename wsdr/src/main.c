#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "dsp/dsp.h"
#include "tools/helpers.h"
#include "web/web_handler.h"
#include "sensor/sensor.h"
#include "tools/timer.h"
#include "dsp/spectrum.h"

static volatile int force_exit = 0;

static void sighandler(int sig)
{
    DEBUG("sighandler %d\n", sig);

    force_exit = 1;
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    // Timing test
    time_test_1();
    time_test_2();
    time_test_3();

    // Signal for Ctrl+C
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);

    INFO("Initializing sensor\n");
    sensor_init();

    INFO("Initializing dsp\n");
    dsp_init();

    INFO("Initializing web service...\n");
    web_init();

    // Loop
    while (!force_exit)
    {
        web_poll();
    }

    INFO("Closing web context\n");
    web_close();

    INFO("Closing dsp\n");
    dsp_close();

    INFO("Closing sensor\n");
    sensor_close();

    return 0;
}
