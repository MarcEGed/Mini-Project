#include "SyncTestUI.h"

#include <Arduino.h>
#include <config.h>
#include <display.h>
#include <protocol.h>
#include <stdio.h>
#include <string.h>
#include <ftimers.h>
#include <transceiver.h>

static uint32_t gNowMs = 0;
static uint32_t gSyncFlashUntil = 0;

// The counter is used to get the channel we should hope to
// counter % HOPPING_CHANNELS_SIZE is the index of the channel in
// HOPPING_CHANNELS we should be on. when we send a sync packet, we should
// restart the timer, same thing should happen when we receive a sync packet, so
// that both sides should be hopping in sync. Propagation delay, and processing
// time is assumed negligible.

static void render() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    const char* title = "SYNC TEST";
    int16_t titleWidth = strlen(title) * 6;
    display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 2);
    display.print(title);

    display.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);

    char line[22];
    snprintf(line, sizeof(line), "Counter: %lu", (unsigned long)readCounter());
    display.setCursor(8, 26);
    display.print(line);

    const int16_t btnW = 64;
    const int16_t btnH = 14;
    const int16_t btnX = (SCREEN_WIDTH - btnW) / 2;
    const int16_t btnY = 44;

    bool highlight = (gSyncFlashUntil != 0) && (gNowMs < gSyncFlashUntil);
    if (highlight) {
        display.fillRect(btnX, btnY, btnW, btnH, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display.drawRect(btnX, btnY, btnW, btnH, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }

    display.setCursor(btnX + 18, btnY + 3);
    display.print("SYNC");

    display.display();
}

bool syncTestUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
                         uint32_t now, uint32_t counter) {
    (void)dirY;

    gNowMs = now;

    if (btnDown) {
        xcvr.sendSync(counter);
        gSyncFlashUntil = now + 200;
        resetCounterTimer();
    }

    NDSyncData incoming = {0};
    if (xcvr.read(PacketType::ND_SYNC, incoming)) {
        setCounter(static_cast<uint32_t>(incoming.timer_val)); 
        gSyncFlashUntil = now + 200;
    }

    return true;
}

void syncTestUIInitDisplay() {
    gNowMs = 0;
    gSyncFlashUntil = 0;
    render();
}

void syncTestUIUpdate() { render(); }
