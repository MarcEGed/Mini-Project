#include "AudioUI.h"

#include <display.h>

#include "audio/audio.h"

enum class AudioRoleChoice : uint8_t {
    Tx = 0,
    Rx = 1,
};

static AudioRoleChoice gRoleChoice = AudioRoleChoice::Tx;
static bool            gRoleSelectionActive = true;

static void renderRoleSelect() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(36, 2);
    display.print("AUDIO TEST");
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

    display.setCursor(24, 24);
    display.print(gRoleChoice == AudioRoleChoice::Tx ? "> TX" : "  TX");

    display.setCursor(24, 38);
    display.print(gRoleChoice == AudioRoleChoice::Rx ? "> RX" : "  RX");

    display.setCursor(0, 54);
    display.print("Press button to start");

    display.display();
}

static void renderRunning() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(36, 2);
    display.print("AUDIO TEST");
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

    display.setCursor(0, 20);
    display.print("Mode:");
    display.setCursor(42, 20);
    display.print(gRoleChoice == AudioRoleChoice::Tx ? "TX" : "RX");

    display.setCursor(0, 36);
    display.print("Streaming audio...");
    display.setCursor(0, 52);
    display.print("BACK = exit");

    display.display();
}

bool audioUITickInput(transceiver& xcvr, int8_t dirY, bool btnDown) {
    if (gRoleSelectionActive) {
        bool changed = false;

        if (dirY != 0) {
            gRoleChoice = (gRoleChoice == AudioRoleChoice::Tx)
                              ? AudioRoleChoice::Rx
                              : AudioRoleChoice::Tx;
            changed = true;
        }

        if (btnDown) {
            const audioMode mode =
                (gRoleChoice == AudioRoleChoice::Tx) ? AUDIO_MODE_TX : AUDIO_MODE_RX;
            audioSetup(xcvr, mode);
            gRoleSelectionActive = false;
            changed = true;
        }

        return changed;
    }

    return false;
}

void audioUIInitDisplay() {
    gRoleSelectionActive = true;
    audioStop();
    audioUIUpdate();
}

void audioUIUpdate() {
    if (gRoleSelectionActive) {
        renderRoleSelect();
        return;
    }

    renderRunning();
}
