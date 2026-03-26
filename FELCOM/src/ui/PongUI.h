#ifndef PONGUI_H
#define PONGUI_H

#include <stdint.h>

#include "pong/pong.h"

enum PongUpdateKind : uint8_t {
    PongMainScreen = 0,
};

void pongUIInitDisplay(PongGame* game);
void pongUIUpdate(PongGame* game, PongUpdateKind kind);

#endif
