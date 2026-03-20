#include "joystick.h"
#include "config.h"

static bool moved = false;
static const uint16_t CENTER = 2048;

void joystickInit(){
    pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
}

int8_t joystickDirection(){
    uint16_t y = analogRead(JOYSTICK_Y_PIN);
    if (y < CENTER - JOYSTICK_DEADZONE) return  1; //up
    if (y > CENTER + JOYSTICK_DEADZONE) return -1; //down
    return 0;
}

bool joystickButtonPressed() {
    static bool lastState = HIGH;
    bool state = digitalRead(JOYSTICK_SW_PIN);
    bool pressed = (state == LOW && lastState == HIGH);
    lastState = state;
    return pressed;
}