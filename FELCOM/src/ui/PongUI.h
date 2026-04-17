#ifndef PONGUI_H
#define PONGUI_H

#include <stdint.h>

#include "pong/pong.h"

enum PongUpdateKind : uint8_t {
    PongAutoScreen = 0,
    PongRoleSelectScreen = 1,
    PongMatchScreen = 2,
    PongGameOverScreen = 3,
};

bool pongUITickInput(PongGame* game, int8_t dirY, bool btnDown);

void pongUIInitDisplay(PongGame* game);
void pongUIUpdate(PongGame* game, PongUpdateKind kind);

#endif
