#include "input.h"
#include <config.h>

// ════════════════════════════════════════════════════════════════════════════
// ROTARY ENCODER
// ════════════════════════════════════════════════════════════════════════════
#if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
static uint8_t  lastClk   = HIGH;
static uint8_t  lastBtn   = HIGH;
static uint32_t lastBtnMs = 0;
static const uint32_t DEBOUNCE_MS = 40;

void inputInit() {
    pinMode(ROTARY_ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_DT_PIN,  INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_SW_PIN,  INPUT_PULLUP);
    lastClk = digitalRead(ROTARY_ENCODER_CLK_PIN);
    lastBtn = digitalRead(ROTARY_ENCODER_SW_PIN);
    //LOG_INFO("Input: rotary encoder ready");
}

int8_t inputDirectionY() {
    uint8_t clk = digitalRead(ROTARY_ENCODER_CLK_PIN);
    int8_t  dir = 0;

    if (clk != lastClk && clk == LOW) {
        dir = (digitalRead(ROTARY_ENCODER_DT_PIN) == HIGH) ? 1 : -1;
        //LOG_INFO("Encoder dir: %d", dir);
    }
    lastClk = clk;
    return dir;
}

bool inputButtonPressed() {
    uint32_t now   = millis();
    uint8_t  state = digitalRead(ROTARY_ENCODER_SW_PIN);

    if (lastBtn == HIGH && state == LOW &&
        (now - lastBtnMs) > DEBOUNCE_MS) {
        lastBtnMs = now;
        lastBtn   = state;
        //LOG_INFO("Encoder button pressed");
        return true;
    }
    lastBtn = state;
    return false;
}

// ════════════════════════════════════════════════════════════════════════════
// JOYSTICK
// ════════════════════════════════════════════════════════════════════════════
#else

static uint8_t  lastBtn   = HIGH;
static uint32_t lastBtnMs = 0;
static const uint32_t DEBOUNCE_MS = 40;

void inputInit() {
    //pin 35 is input-only on ESP32 and no pinMode needed for ADC
    pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
    //LOG_INFO("Input: joystick ready (polling)");
}

int8_t inputDirectionY() {
    int raw      = analogRead(JOYSTICK_Y_PIN); //0-4095
    int centered = raw - 2048;
    //LOG_INFO("Joystick raw: %d centered: %d", raw, centered);
    if (centered >  JOYSTICK_DEADZONE) return  1;
    if (centered < -JOYSTICK_DEADZONE) return -1;
    return 0;
}

bool inputButtonPressed() {
    uint32_t now   = millis();
    uint8_t  state = digitalRead(JOYSTICK_SW_PIN);

    if (lastBtn == HIGH && state == LOW &&
        (now - lastBtnMs) > DEBOUNCE_MS) {
        lastBtnMs = now;
        lastBtn   = state;
        //LOG_INFO("Joystick button pressed");
        return true;
    }
    lastBtn = state;
    return false;
}

#endif