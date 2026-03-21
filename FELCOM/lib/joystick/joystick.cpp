#include "joystick.h"

#include <config.h>

static const uint16_t CENTER = 2048;

void joystickInit()
{
#if !defined(USE_ROTARY_ENCODER) || !USE_ROTARY_ENCODER
    pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
#endif
}

int8_t joystickDirectionY()
{
#if !defined(USE_ROTARY_ENCODER) || !USE_ROTARY_ENCODER
    uint16_t y = analogRead(JOYSTICK_Y_PIN);
    if (y < CENTER - JOYSTICK_DEADZONE)
        return 1; // up
    if (y > CENTER + JOYSTICK_DEADZONE)
        return -1; // down
    return 0;
#else
    return 0;
#endif
}

bool joystickButtonPressed()
{
#if !defined(USE_ROTARY_ENCODER) || !USE_ROTARY_ENCODER
    static bool lastState = HIGH;
    bool state = digitalRead(JOYSTICK_SW_PIN);
    bool pressed = (state == LOW && lastState == HIGH);
    lastState = state;
    return pressed;
#else
    return false;
#endif
}