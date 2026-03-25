#ifndef PONG_H
#define PONG_H

#include <stdbool.h>

struct PongGame {
    static constexpr int kBoundaryTop = 2;
    static constexpr int kBoundaryBottom = 61;
    static constexpr int kBoundaryLeft = 0;
    static constexpr int kBoundaryRight = 127;
    static constexpr int kPaddleHeight = 15;
    static constexpr int kPlayfieldWidth = kBoundaryRight - kBoundaryLeft + 1;
    static constexpr int kPlayfieldHeight = kBoundaryBottom - kBoundaryTop + 1;
    static constexpr int kBorderTopY = kBoundaryTop - 2;
    static constexpr int kBorderBottomY = kBoundaryBottom + 2;
    static constexpr int kCenterX = (kBoundaryLeft + kBoundaryRight) / 2;
    static constexpr int kLeftPaddleX = kBoundaryLeft + 1;
    static constexpr int kRightPaddleX = kBoundaryRight - 1;
    static constexpr int kInitialBallX = kCenterX;
    static constexpr int kInitialBallY = (kBoundaryTop + kBoundaryBottom) / 2;
    static constexpr int kInitialPaddleY = 24;

    bool isRunning;

    int scoreLeft;
    int scoreRight;

    bool ballMovingRight;
    int ballX;
    int ballY;

    int paddleLeft;
    int paddleRight;

    void moveBall();
};

// TODO: consider making these member functions of PongGame instead of free
// functions
void initializeGame(PongGame* g);
void drawGame(PongGame* game);
void moveLeftPaddle(PongGame* game, int delta);
void moveRightPaddle(PongGame* game, int delta);

#endif  // PONG_H