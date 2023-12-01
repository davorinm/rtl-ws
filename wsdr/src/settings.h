#ifndef SETTINGS_H
#define SETTINGS_H

// Web port
#define PORT 8090

// HTML default path
#define MAIN_HTML "/index.html"

// HTML resources
#define HTML_PATH "./resources"

// Bandwidth
#define DECIMATED_TARGET_BW_HZ  (44100 * 4)

// Sensors
#define SENSOR_RTLSDR 0
#define SENSOR_PLUTO 1

// RTL SDR
#define RTL_DEFAULT_SAMPLE_RATE 2048000
#define RTL_DEFAULT_FREQUENCY 102800000

#endif