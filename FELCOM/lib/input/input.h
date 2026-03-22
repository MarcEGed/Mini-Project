#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

void inputInit();

int8_t inputDirectionY(); //-1, 0  or 1
bool inputButtonPressed(); //debounced button check

#endif