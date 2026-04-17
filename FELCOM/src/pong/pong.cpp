#include "pong.h"

#include <config.h>
#include <debug.h>
#include <transceiver.h>

struct PongPacket {
    static constexpr uint8_t kTypeState = 0x01;
    static constexpr uint8_t kTypeInput = 0x02;

    uint8_t type;
    uint8_t reserved[3];
    int16_t paddleY;  // sender's paddle Y
    int16_t ballX;    // only meaningful from host
    int16_t ballY;    // only meaningful from host
    int8_t ballVX;
    int8_t ballVY;
    uint8_t scoreHost;
    uint8_t scoreGuest;
};

static PongGame sDefaultGame{};
static PongGame* sActiveGame = &sDefaultGame;

extern transceiver xcvr;

static PongGame* resolveGame(PongGame* game) {
    if (game != nullptr) {
        sActiveGame = game;
        return game;
    }

    if (sActiveGame == nullptr) {
        sActiveGame = &sDefaultGame;
    }
    return sActiveGame;
}

static void clampPaddle(float& y) {
    if (y < 0.0f) {
        y = 0.0f;
    }
    const float maxPaddleY =
        static_cast<float>(PongGame::kScreenHeight - PongGame::kPaddleHeight);
    if (y > maxPaddleY) {
        y = maxPaddleY;
    }
}

static void resetBall(PongGame* game, bool toHost) {
    game->ballX = PongGame::kScreenWidth / 2.0f;
    game->ballY = PongGame::kScreenHeight / 2.0f;
    game->ballVX = toHost ? -2.5f : 2.5f;
    game->ballVY = 1.5f;
    game->ballMovingRight = (game->ballVX > 0.0f);
}

void initializeGame(PongGame* game) {
    if (game == nullptr) {
        return;
    }

    const bool isHost = game->isHost;

    game->isRunning = true;
    game->isHost = isHost;
    game->gameOver = false;

    game->scoreLeft = 0;
    game->scoreRight = 0;

    game->ballMovingRight = true;
    game->ballX = static_cast<float>(PongGame::kInitialBallX);
    game->ballY = static_cast<float>(PongGame::kInitialBallY);
    game->ballVX = 0.0f;
    game->ballVY = 0.0f;

    game->paddleLeft = static_cast<float>(PongGame::kInitialPaddleY);
    game->paddleRight = static_cast<float>(PongGame::kInitialPaddleY);

    game->lastTickMs = 0;
    game->lastTxMs = 0;
}

static void sendState(const PongGame* game) {
    PongPacket pkt{};

    if (game->isHost) {
        pkt.type = PongPacket::kTypeState;
        pkt.paddleY = static_cast<int16_t>(game->paddleLeft);
        pkt.ballX = static_cast<int16_t>(game->ballX);
        pkt.ballY = static_cast<int16_t>(game->ballY);
        pkt.ballVX = static_cast<int8_t>(game->ballVX);
        pkt.ballVY = static_cast<int8_t>(game->ballVY);
        pkt.scoreHost = game->scoreLeft;
        pkt.scoreGuest = game->scoreRight;
    } else {
        pkt.type = PongPacket::kTypeInput;
        pkt.paddleY = static_cast<int16_t>(game->paddleRight);
    }

    xcvr.setMode(TRANSMIT);
    xcvr.write(&pkt, sizeof(PongPacket));
    xcvr.setMode(RECEIVE);
}

static void receivePackets(PongGame* game) {
    PongPacket pkt{};

    for (uint8_t i = 0; i < 4; i++) {
        if (!xcvr.read(&pkt, sizeof(PongPacket))) {
            break;
        }

        if (game->isHost) {
            if (pkt.type == PongPacket::kTypeInput) {
                game->paddleRight = static_cast<float>(pkt.paddleY);
                clampPaddle(game->paddleRight);
            }
            continue;
        }

        if (pkt.type == PongPacket::kTypeState) {
            game->paddleLeft = static_cast<float>(pkt.paddleY);
            game->ballX = static_cast<float>(pkt.ballX);
            game->ballY = static_cast<float>(pkt.ballY);
            game->ballVX = static_cast<float>(pkt.ballVX);
            game->ballVY = static_cast<float>(pkt.ballVY);
            game->scoreLeft = pkt.scoreHost;
            game->scoreRight = pkt.scoreGuest;
            game->ballMovingRight = (game->ballVX > 0.0f);
            if (game->scoreLeft >= PONG_SCORE_TO_WIN ||
                game->scoreRight >= PONG_SCORE_TO_WIN) {
                game->gameOver = true;
                game->isRunning = false;
            }
        }
    }
}

static void tickPhysics(PongGame* game) {
    game->ballX += game->ballVX;
    game->ballY += game->ballVY;

    if (game->ballY <= 0.0f) {
        game->ballY = 0.0f;
        game->ballVY = -game->ballVY;
    }
    const float maxBallY =
        static_cast<float>(PongGame::kScreenHeight - PongGame::kBallSize);
    if (game->ballY >= maxBallY) {
        game->ballY = maxBallY;
        game->ballVY = -game->ballVY;
    }

    if (game->ballVX < 0.0f &&
        game->ballX <= static_cast<float>(PongGame::kPaddleMargin +
                                          PongGame::kPaddleWidth) &&
        game->ballY + PongGame::kBallSize >= game->paddleLeft &&
        game->ballY <= game->paddleLeft + PongGame::kPaddleHeight) {
        game->ballVX = -game->ballVX * 1.05f;
        game->ballX = static_cast<float>(PongGame::kPaddleMargin +
                                         PongGame::kPaddleWidth);
    }

    if (game->ballVX > 0.0f &&
        game->ballX + PongGame::kBallSize >=
            static_cast<float>(PongGame::kScreenWidth -
                               PongGame::kPaddleMargin -
                               PongGame::kPaddleWidth) &&
        game->ballY + PongGame::kBallSize >= game->paddleRight &&
        game->ballY <= game->paddleRight + PongGame::kPaddleHeight) {
        game->ballVX = -game->ballVX * 1.05f;
        game->ballX = static_cast<float>(
            PongGame::kScreenWidth - PongGame::kPaddleMargin -
            PongGame::kPaddleWidth - PongGame::kBallSize);
    }

    if (game->ballX < 0.0f) {
        game->scoreRight++;
        if (game->scoreRight >= PONG_SCORE_TO_WIN) {
            game->gameOver = true;
            game->isRunning = false;
            return;
        }
        resetBall(game, false);
    }

    if (game->ballX > static_cast<float>(PongGame::kScreenWidth)) {
        game->scoreLeft++;
        if (game->scoreLeft >= PONG_SCORE_TO_WIN) {
            game->gameOver = true;
            game->isRunning = false;
            return;
        }
        resetBall(game, true);
    }

    game->ballMovingRight = (game->ballVX > 0.0f);
}

void pongSetup(PongGame* game) {
    game = resolveGame(game);

    initializeGame(game);
    game->lastTickMs = millis();
    game->lastTxMs = game->lastTickMs;
    resetBall(game, true);

    xcvr.setMode(RECEIVE);
    LOG_INFO("Pong start, role=%s", game->isHost ? "HOST" : "GUEST");
}

void pongLoop(PongGame* game, int8_t localDirY) {
    game = resolveGame(game);

    if (!game->isRunning || game->gameOver) {
        return;
    }

    const uint32_t now = millis();

    receivePackets(game);

    if (game->gameOver) {
        return;
    }

    if (game->isHost) {
        game->paddleLeft += localDirY * 2.5f;
        clampPaddle(game->paddleLeft);
    } else {
        game->paddleRight += localDirY * 2.5f;
        clampPaddle(game->paddleRight);
    }

    if (game->isHost && (now - game->lastTickMs >= PONG_TICK_MS)) {
        game->lastTickMs = now;
        tickPhysics(game);
    }

    if (now - game->lastTxMs >= PONG_TX_INTERVAL_MS) {
        game->lastTxMs = now;
        sendState(game);
    }
}

void pongLoop(PongGame* game) { pongLoop(game, 0); }

void pongSetup() { pongSetup(nullptr); }

void pongLoop() { pongLoop(nullptr, 0); }
