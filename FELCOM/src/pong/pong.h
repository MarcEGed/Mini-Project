#ifndef PONG_H
#define PONG_H
#include <config.h>
#include <stdbool.h>
#include <stdint.h>

struct PongGame {
    static constexpr int kScreenWidth = SCREEN_WIDTH;
    static constexpr int kScreenHeight = SCREEN_HEIGHT;
    static constexpr int kPaddleWidth = 3;
    static constexpr int kBoundaryTop = 2;
    static constexpr int kBoundaryBottom = kScreenHeight - 3;
    static constexpr int kBoundaryLeft = 0;
    static constexpr int kBoundaryRight = kScreenWidth - 1;
    static constexpr int kPaddleHeight = 12;
    static constexpr int kBallSize = 3;
    static constexpr int kPaddleMargin = 4;
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

    bool isRunning = false;
    // Set by caller (for example UI) before calling pongSetup.
    bool isHost = false;
    bool gameOver = false;

    uint8_t scoreLeft = 0;
    uint8_t scoreRight = 0;

    bool ballMovingRight = true;
    float ballX = static_cast<float>(kInitialBallX);
    float ballY = static_cast<float>(kInitialBallY);
    float ballVX = 0.0f;
    float ballVY = 0.0f;

    float paddleLeft = static_cast<float>(kInitialPaddleY);
    float paddleRight = static_cast<float>(kInitialPaddleY);

    uint32_t lastTickMs = 0;
    uint32_t lastTxMs = 0;

    void moveBall();
};

// TODO: consider making these member functions of PongGame instead of free
// functions
void initializeGame(PongGame* g);

void pongSetup(PongGame* game);
void pongLoop(PongGame* game, int8_t localDirY);
void pongLoop(PongGame* game);

void pongSetup();
void pongLoop();

#endif