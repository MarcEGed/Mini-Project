#ifndef JOYSTICK_H
#define JOYSTICK_H
#include <config.h>
#include <Arduino.h>

#define JOYSTICK_DEADZONE 200

void joystickInit();
int8_t joystickDirectionY();
bool joystickButtonPressed();

#endif