#ifndef SYNC_UI_H
#define SYNC_UI_H

#include <stdint.h>

struct transceiver;

bool syncUITickInput(transceiver& xcvr, bool btnDown, uint32_t now_ms);
void syncUIInitDisplay(const transceiver* xcvr);
void syncUIUpdate(const transceiver* xcvr);

#endif
