#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

void inputInit();

//-1, 0  or 1
int8_t inputDirectionY();
// Immediate -1/0/1 direction, intended for realtime gameplay.
int8_t inputDirectionYContinuous();
// debounced button check
bool inputButtonPressed();

#endif