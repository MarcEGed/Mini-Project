#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

void inputInit();

//-1, 0  or 1
int8_t inputDirectionY();
// debounced button check
bool inputButtonPressed();

#endif