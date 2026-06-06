#include "AudioUI.h"

#include <config.h>
#include <display.h>
#include <string.h>

#include "audio.h"

static void render() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    const char* title = "AUDIO";
    int16_t titleWidth = strlen(title) * 6;
    display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 2);
    display.print(title);
    display.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);

    // Big role indicator.
    bool talking = audio::isTalking();
    const char* state = talking ? "TALKING" : "LISTENING";
    display.setTextSize(2);
    int16_t w = strlen(state) * 12;
    display.setCursor((SCREEN_WIDTH - w) / 2, 26);
    display.print(state);

    display.setTextSize(1);
    display.setCursor(4, 54);
    display.print("SEL talk   BACK exit");

    display.display();
}

bool audioUITickInput(int8_t dirY, bool btnDown) {
    (void)dirY;

    bool changed = false;
    if (btnDown) {
        audio::setTalking(!audio::isTalking());
        changed = true;  // role changed -> safe to redraw once
    }

    // Cooperative, non-blocking pump (capture+TX when talking, RX+play when
    // listening). Runs every loop; must stay fast so FHSS hops on time.
    audio::update();

    return changed;
}

void audioUIInitDisplay() { render(); }

void audioUIUpdate() { render(); }
