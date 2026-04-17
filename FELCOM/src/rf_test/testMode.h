#ifndef RF_TEST_H
#define RF_TEST_H

#include <stdint.h>
#include <transceiver.h>

struct RFTestStats {
    uint32_t sent = 0;
    uint32_t echoed = 0;
    uint32_t lost = 0;
    uint32_t corrupt = 0;
    uint32_t received = 0;
};

void rfTestSetup(transceiver& xcvr);
void rfTestLoop(transceiver& xcvr);
bool rfTestTick(transceiver& xcvr, uint32_t now);

void rfTestSetTxMode(bool txMode);
const RFTestStats& rfTestGetStats();
bool rfTestIsTxMode();

#endif