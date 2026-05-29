#include "SyncUI.h"

#include <config.h>
#include <display.h>
#include <stdio.h>
#include <transceiver.h>

enum class SyncState : uint8_t {
    Idle = 0,
    Scanning = 1,
    Synced = 2,
    NewNetwork = 3,
};

static SyncState gSyncState = SyncState::Idle;

static void renderSync(const transceiver* xcvr) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
    display.drawFastHLine(2, 17, SCREEN_WIDTH - 4, SSD1306_WHITE);

    const char* title = "SYNCHRONIZE";
    int16_t titleWidth = strlen(title) * 6;
    display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 6);
    display.print(title);

    const uint8_t lineY = 22;
    const uint8_t lineStep = 11;

    switch (gSyncState) {
        case SyncState::Idle:
            display.setCursor(6, lineY);
            if (xcvr && xcvr->joined) {
                display.print("Status: Synced");
                char buf[20];
                snprintf(buf, sizeof(buf), "Peers: %u",
                         (unsigned)xcvr->syncedPeerCount());
                display.setCursor(6, lineY + lineStep);
                display.print(buf);
            } else {
                display.print("Status: No Sync");
            }
            display.setCursor(6, 54);
            display.print("[Btn] Sync now");
            break;

        case SyncState::Scanning:
            display.setCursor(6, lineY);
            display.print("Scanning...");
            display.setCursor(6, lineY + lineStep);
            display.print("Ch: 110");
            break;

        case SyncState::Synced: {
            display.setCursor(6, lineY);
            display.print("Synced!");
            if (xcvr) {
                char buf[20];
                snprintf(buf, sizeof(buf), "Peers: %u",
                         (unsigned)xcvr->syncedPeerCount());
                display.setCursor(6, lineY + lineStep);
                display.print(buf);
            }
            display.setCursor(6, 54);
            display.print("[Btn] Resync");
            break;
        }

        case SyncState::NewNetwork:
            display.setCursor(6, lineY);
            display.print("New Network");
            display.setCursor(6, lineY + lineStep);
            display.print("No peers found");
            display.setCursor(6, 54);
            display.print("[Btn] Resync");
            break;
    }

    display.display();
}

void syncUIInitDisplay(const transceiver* xcvr) {
    gSyncState = SyncState::Idle;
    renderSync(xcvr);
}

bool syncUITickInput(transceiver& xcvr, bool btnDown, uint32_t now_ms) {
    bool changed = false;

    if (gSyncState == SyncState::Scanning) {
        if (xcvr.joined) {
            gSyncState = xcvr.syncedPeerCount() > 0 ? SyncState::Synced
                                                     : SyncState::NewNetwork;
            changed = true;
        }
        return changed;
    }

    if (btnDown) {
        xcvr.triggerManualSync(now_ms);
        gSyncState = SyncState::Scanning;
        changed = true;
    }

    return changed;
}

void syncUIUpdate(const transceiver* xcvr) { renderSync(xcvr); }
