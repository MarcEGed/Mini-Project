#include "PongUI.h"

#include <display.h>

void drawGame(PongGame* game) {
    display.clearDisplay();

    display.drawFastHLine(PongGame::kBoundaryLeft, PongGame::kBorderTopY,
                          PongGame::kPlayfieldWidth, 1);
    display.drawFastHLine(PongGame::kBoundaryLeft, PongGame::kBorderBottomY,
                          PongGame::kPlayfieldWidth, 1);

    display.drawFastVLine(PongGame::kCenterX, PongGame::kBoundaryTop,
                          PongGame::kPlayfieldHeight, 1);

    display.drawFastVLine(PongGame::kLeftPaddleX, game->paddleLeft,
                          PongGame::kPaddleHeight + 1, 1);
    display.drawFastVLine(PongGame::kRightPaddleX, game->paddleRight,
                          PongGame::kPaddleHeight + 1, 1);

    display.drawCircle(game->ballX, game->ballY, 1, 1);

    display.display();
}

void pongUIInitDisplay(PongGame* game) {
    if (game == nullptr) {
        return;
    }
    drawGame(game);
}

void pongUIUpdate(PongGame* game, PongUpdateKind kind) {
    if (game == nullptr) {
        return;
    }

    (void)kind;
    drawGame(game);
}
