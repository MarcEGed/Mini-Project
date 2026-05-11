#ifndef AUDIO_UI_H
#define AUDIO_UI_H

#include <stdint.h>

struct transceiver;

bool audioUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown);
void audioUIInitDisplay();
void audioUIUpdate();

#endif
