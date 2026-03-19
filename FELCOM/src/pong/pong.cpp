#include "pong.h"

#include "display/display.h"

void initializeGame(PongGame *g)
{
    g->isRunning = true;
    g->scoreLeft = 0;
    g->scoreRight = 0;
    g->ballMovingRight = true;
    g->ballX = PongGame::kInitialBallX;
    g->ballY = PongGame::kInitialBallY;
    g->paddleLeft = PongGame::kInitialPaddleY;
    g->paddleRight = PongGame::kInitialPaddleY;
}

void drawGame(PongGame *game)
{

    display.clearDisplay();

    display.drawFastHLine(PongGame::kBoundaryLeft, PongGame::kBorderTopY, PongGame::kPlayfieldWidth, 1);
    display.drawFastHLine(PongGame::kBoundaryLeft, PongGame::kBorderBottomY, PongGame::kPlayfieldWidth, 1);

    display.drawFastVLine(PongGame::kCenterX, PongGame::kBoundaryTop, PongGame::kPlayfieldHeight, 1);

    display.drawFastVLine(PongGame::kLeftPaddleX, game->paddleLeft, PongGame::kPaddleHeight + 1, 1);
    display.drawFastVLine(PongGame::kRightPaddleX, game->paddleRight, PongGame::kPaddleHeight + 1, 1);

    display.drawCircle(game->ballX, game->ballY, 1, 1);

    display.display();
}

void moveLeftPaddle(PongGame *game, int delta)
{
    game->paddleLeft += delta;
    if (game->paddleLeft < PongGame::kBoundaryTop)
    {
        game->paddleLeft = PongGame::kBoundaryTop;
    }
    else if (game->paddleLeft + PongGame::kPaddleHeight > PongGame::kBoundaryBottom)
    {
        game->paddleLeft = PongGame::kBoundaryBottom - PongGame::kPaddleHeight;
    }
}

void moveRightPaddle(PongGame *game, int delta)
{
    game->paddleRight += delta;
    if (game->paddleRight < PongGame::kBoundaryTop)
    {
        game->paddleRight = PongGame::kBoundaryTop;
    }
    else if (game->paddleRight + PongGame::kPaddleHeight > PongGame::kBoundaryBottom)
    {
        game->paddleRight = PongGame::kBoundaryBottom - PongGame::kPaddleHeight;
    }
}