#include "RFTestUI.h"

#include <display.h>
#include <stdio.h>

#include "rf_test/testMode.h"

enum class RFTestRoleChoice : uint8_t {
    Tx = 0,
    Rx = 1,
};

static RFTestRoleChoice gRoleChoice = RFTestRoleChoice::Tx;
static bool gRoleSelectionActive = true;

static void renderRoleSelect() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(34, 2);
    display.print("RF TEST");
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

    display.setCursor(24, 24);
    display.print(gRoleChoice == RFTestRoleChoice::Tx ? "> TX" : "  TX");

    display.setCursor(24, 38);
    display.print(gRoleChoice == RFTestRoleChoice::Rx ? "> RX" : "  RX");

    display.setCursor(0, 54);
    display.print("Press button to start");

    display.display();
}

static void renderTx(const RFTestStats& stats) {
    float ber = stats.echoed > 0 ? 100.0f * stats.corrupt / stats.echoed : 0.0f;
    float lossP = stats.sent > 0 ? 100.0f * stats.lost / stats.sent : 0.0f;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(3);
    char berStr[8];
    snprintf(berStr, sizeof(berStr), "%.0f%%", ber);
    display.setCursor(0, 0);
    display.print(berStr);

    display.setTextSize(1);
    display.setCursor(80, 4);
    display.print("BER");

    display.drawFastHLine(0, 26, 128, SSD1306_WHITE);

    char line[22];
    snprintf(line, sizeof(line), "Sent:  %lu", (unsigned long)stats.sent);
    display.setCursor(0, 30);
    display.print(line);

    snprintf(line, sizeof(line), "Echo:  %lu", (unsigned long)stats.echoed);
    display.setCursor(0, 40);
    display.print(line);

    snprintf(line, sizeof(line), "Lost:  %lu (%.0f%%)",
             (unsigned long)stats.lost, lossP);
    display.setCursor(0, 50);
    display.print(line);

    display.display();
}

static void renderRx(const RFTestStats& stats) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("RX MODE");

    display.drawFastHLine(0, 20, 128, SSD1306_WHITE);

    char line[22];
    snprintf(line, sizeof(line), "Rcvd:   %lu", (unsigned long)stats.received);
    display.setCursor(0, 26);
    display.print(line);

    snprintf(line, sizeof(line), "Bad:    %lu", (unsigned long)stats.corrupt);
    display.setCursor(0, 38);
    display.print(line);

    float ber =
        stats.received > 0 ? 100.0f * stats.corrupt / stats.received : 0.0f;
    snprintf(line, sizeof(line), "BER:    %.1f%%", ber);
    display.setCursor(0, 50);
    display.print(line);

    display.display();
}

bool rfTestUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown,
                       uint32_t now) {
    if (gRoleSelectionActive) {
        bool changed = false;

        if (dirY != 0) {
            gRoleChoice = (gRoleChoice == RFTestRoleChoice::Tx)
                              ? RFTestRoleChoice::Rx
                              : RFTestRoleChoice::Tx;
            changed = true;
        }

        if (btnDown) {
            rfTestSetTxMode(gRoleChoice == RFTestRoleChoice::Tx);
            rfTestSetup(xcvr);
            gRoleSelectionActive = false;
            changed = true;
        }

        return changed;
    }

    if (btnDown) {
        gRoleSelectionActive = true;
        return true;
    }

    return rfTestTick(xcvr, now);
}

void rfTestUIInitDisplay() {
    gRoleChoice =
        rfTestIsTxMode() ? RFTestRoleChoice::Tx : RFTestRoleChoice::Rx;
    gRoleSelectionActive = true;
    rfTestUIUpdate(RFTestAutoScreen);
}

void rfTestUIUpdate(RFTestUpdateKind kind) {
    const RFTestStats& stats = rfTestGetStats();

    if (kind == RFTestAutoScreen) {
        if (gRoleSelectionActive) {
            kind = RFTestModeSelectScreen;
        } else {
            kind = rfTestIsTxMode() ? RFTestTxScreen : RFTestRxScreen;
        }
    }

    if (kind == RFTestModeSelectScreen) {
        renderRoleSelect();
        return;
    }

    if (kind == RFTestTxScreen) {
        renderTx(stats);
        return;
    }

    renderRx(stats);
}
