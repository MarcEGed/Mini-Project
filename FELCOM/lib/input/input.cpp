#include "input.h"

#include <Arduino.h>
#include <config.h>

// ════════════════════════════════════════════════════════════════════════════
// buttons
// ════════════════════════════════════════════════════════════════════════════
// ======================================================
// BUTTON MODE
// ======================================================

static const uint32_t DEBOUNCE_MS = 40;
static const uint32_t HOLD_REPEAT_MS = 150;

static uint32_t lastUpMs = 0;
static uint32_t lastDownMs = 0;
static uint32_t lastSelectMs = 0;
static uint32_t lastBackMs = 0;

static uint8_t lastSelectState = HIGH;
static uint8_t lastBackState = HIGH;

static uint32_t lastMoveMs = 0;
static int8_t lastDir = 0;

void inputInit() {
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
    pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
    pinMode(BTN_BACK_PIN, INPUT_PULLUP);
}

int8_t inputDirectionY() {
    uint32_t now = millis();

    bool up = (digitalRead(BTN_UP_PIN) == LOW);
    bool down = (digitalRead(BTN_DOWN_PIN) == LOW);

    int8_t dir = 0;
    if (up)
        dir = 1;
    else if (down)
        dir = -1;

    if (dir == 0) {
        lastDir = 0;
        return 0;
    }

    if (dir != lastDir || (now - lastMoveMs) >= HOLD_REPEAT_MS) {
        if ((dir == 1 && (now - lastUpMs) > DEBOUNCE_MS) ||
            (dir == -1 && (now - lastDownMs) > DEBOUNCE_MS)) {
            lastMoveMs = now;
            lastDir = dir;

            if (dir == 1) lastUpMs = now;
            if (dir == -1) lastDownMs = now;

            return dir;
        }
    }

    return 0;
}

bool inputButtonPressed() {
    uint8_t current = digitalRead(BTN_SELECT_PIN);

    if (lastSelectState == HIGH && current == LOW) {
        lastSelectState = current;
        return true;  // 🔥 trigger ONLY once on press
    }

    lastSelectState = current;
    return false;
}

bool inputBackPressed() {
    uint8_t current = digitalRead(BTN_BACK_PIN);

    if (lastBackState == HIGH && current == LOW) {
        lastBackState = current;
        return true;  // 🔥 trigger ONLY once on press
    }

    lastBackState = current;
    return false;
}

int8_t inputDirectionYContinuous() {
    bool up = (digitalRead(BTN_UP_PIN) == LOW);
    bool down = (digitalRead(BTN_DOWN_PIN) == LOW);

    int8_t dir = 0;
    if (up)
        dir = 1;
    else if (down)
        dir = -1;

    if (dir == 0) {
        lastDir = 0;
        return 0;
    }
    return dir;
}
