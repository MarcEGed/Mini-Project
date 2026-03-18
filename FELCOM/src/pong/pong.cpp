#include "pong.h"

#include "display/display.h"


void drawGame(GamePacket *game)
{

    display.clearDisplay();

    display.drawLine(0, 0, 127, 0, 1);
    display.drawLine(0, 63, 127, 63, 1);

    // TODO: Explore the drawFastHLine
    display.drawLine(64, 2, 64, 61, 1);

    display.drawLine(1, game->paddleLeft, 1, game->paddleLeft + 15, 1);
    display.drawLine(126, game->paddleRight, 126, game->paddleRight + 15, 1);

    display.drawCircle(game->ballX, game->ballY, 1, 1);

    display.display();
}
