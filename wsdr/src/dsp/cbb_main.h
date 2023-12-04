#ifndef CBB_MAIN_H
#define CBB_MAIN_H

#include "rf_decimator.h"

void cbb_init();

rf_decimator *cbb_rf_decimator();

int cbb_new_spectrum_available();

int cbb_get_spectrum_payload(char *buf, int buf_len, double spectrum_gain_db);

void cbb_close();

#endif