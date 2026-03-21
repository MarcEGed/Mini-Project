#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

void joystickInit();
int8_t joystickDirectionY();
bool joystickButtonPressed();

#endif