#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

struct transceiver;

enum audioMode {
    AUDIO_MODE_RX,
    AUDIO_MODE_TX
};

void audioSetup(transceiver& xcvr, audioMode mode);
void audioLoop();
void audioStop();
bool audioIsRunning();

#endif
