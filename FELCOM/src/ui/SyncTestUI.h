#ifndef SYNC_TEST_UI_H
#define SYNC_TEST_UI_H

#include <stdint.h>

struct transceiver;

bool syncTestUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
						 uint32_t now);

void syncTestUIInitDisplay();
void syncTestUIUpdate();

#endif
