#include "input.h"

#include <Arduino.h>
#include <config.h>

// ════════════════════════════════════════════════════════════════════════════
// buttons
// ════════════════════════════════════════════════════════════════════════════
// ======================================================
// BUTTON MODE
// ======================================================
#if defined(USE_BUTTONS) && USE_BUTTONS

static const uint32_t DEBOUNCE_MS = 40;
static const uint32_t HOLD_REPEAT_MS = 150;

static uint32_t lastUpMs = 0;
static uint32_t lastDownMs = 0;
static uint32_t lastSelectMs = 0;
static uint32_t lastBackMs = 0;

static uint8_t lastSelectState = HIGH;
static uint8_t lastBackState   = HIGH;

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
    if (up) dir = 1;
    else if (down) dir = -1;

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
        return true;   // 🔥 trigger ONLY once on press
    }

    lastSelectState = current;
    return false;
}

bool inputBackPressed() {
    uint8_t current = digitalRead(BTN_BACK_PIN);

    if (lastBackState == HIGH && current == LOW) {
        lastBackState = current;
        return true;   // 🔥 trigger ONLY once on press
    }

    lastBackState = current;
    return false;
}

int8_t inputDirectionYContinuous() { return inputDirectionY(); }


// ════════════════════════════════════════════════════════════════════════════
// JOYSTICK
// ════════════════════════════════════════════════════════════════════════════
#else

static uint8_t lastBtn = HIGH;
static uint32_t lastBtnMs = 0;
static const uint32_t DEBOUNCE_MS = 40;

static int8_t readJoystickDirection() {
    int raw = analogRead(JOYSTICK_Y_PIN);
    int centered = raw - JOYSTICK_Y_CENTER;

    if (centered > JOYSTICK_DEADZONE) return 1;
    if (centered < -JOYSTICK_DEADZONE) return -1;
    return 0;
}

void inputInit() {
    // pin 35 is input-only on ESP32 and no pinMode needed for ADC
    pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
    // LOG_INFO("Input: joystick ready (polling)");
}

int8_t inputDirectionY() {
    int8_t dir = readJoystickDirection();

    static int8_t lastDir = 0;
    static uint32_t lastMoveMs = 0;
    static const uint32_t HOLD_REPEAT_MS = 150;  // tune to taste

    uint32_t now = millis();

    if (dir == 0) {
        lastDir = 0;  // reset when joystick is centered
        return 0;
    }

    // only emit a tick if direction changed or enough time passed
    if (dir != lastDir || (now - lastMoveMs) >= HOLD_REPEAT_MS) {
        lastDir = dir;
        lastMoveMs = now;

        return dir;
    }
    return 0;
}

int8_t inputDirectionYContinuous() { return readJoystickDirection(); }

bool inputButtonPressed() {
    uint32_t now = millis();
    uint8_t state = digitalRead(JOYSTICK_SW_PIN);

    if (lastBtn == HIGH && state == LOW && (now - lastBtnMs) > DEBOUNCE_MS) {
        lastBtnMs = now;
        lastBtn = state;
        // LOG_INFO("Joystick button pressed");
        return true;
    }
    lastBtn = state;
    return false;
}

bool inputBackPressed() { return false; }

#endif