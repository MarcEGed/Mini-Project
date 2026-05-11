#ifndef RF_TEST_UI_H
#define RF_TEST_UI_H

#include <stdint.h>

struct transceiver;

enum RFTestUpdateKind : uint8_t {
    RFTestAutoScreen = 0,
    RFTestModeSelectScreen = 1,
    RFTestTxScreen = 2,
    RFTestRxScreen = 3,
};

bool rfTestUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
                       uint32_t now);

void rfTestUIInitDisplay();
void rfTestUIUpdate(RFTestUpdateKind kind);

#endif
