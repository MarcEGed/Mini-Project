#ifndef RF_TEST_H
#define RF_TEST_H
#include "config.h"
#include "debug.h"
#include "display.h"
#include <Arduino.h>
#include <string.h>

#include "transceiver.h"

void rfTestSetup(transceiver& xcvr);
void rfTestLoop(transceiver& xcvr);

#endif