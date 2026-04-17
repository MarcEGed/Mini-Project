#include "PongUI.h"

#include <display.h>
#include <input.h>
#include <stdio.h>

enum class PongRoleChoice : uint8_t {
    Host = 0,
    Join = 1,
};

static PongRoleChoice gRoleChoice = PongRoleChoice::Host;
static bool gRoleSelectionActive = true;

static void drawPaddle(int16_t x, int16_t y) {
    for (int16_t col = 0; col < PongGame::kPaddleWidth; ++col) {
        display.drawFastVLine(x + col, y, PongGame::kPaddleHeight,
                              SSD1306_WHITE);
    }
}

static void renderRoleSelect() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(40, 2);
    display.print("PONG");

    display.drawFastHLine(0, 12, PongGame::kScreenWidth, SSD1306_WHITE);

    display.setCursor(20, 20);
    display.print("Choose your role");

    display.setCursor(24, 34);
    display.print(gRoleChoice == PongRoleChoice::Host ? "> HOST" : "  HOST");

    display.setCursor(24, 46);
    display.print(gRoleChoice == PongRoleChoice::Join ? "> JOIN" : "  JOIN");

    display.setCursor(4, 56);
    display.print("Press button to start");

    display.display();
}

static void renderMatch(const PongGame* game) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    char scoreLine[8];
    snprintf(scoreLine, sizeof(scoreLine), "%u  %u", game->scoreLeft,
             game->scoreRight);
    display.setCursor(PongGame::kScreenWidth / 2 - 12, 0);
    display.print(scoreLine);

    for (uint8_t y = 8; y < PongGame::kScreenHeight; y += 6) {
        display.drawPixel(PongGame::kScreenWidth / 2, y, SSD1306_WHITE);
    }

    drawPaddle(PongGame::kPaddleMargin, static_cast<int16_t>(game->paddleLeft));

    drawPaddle(PongGame::kScreenWidth - PongGame::kPaddleMargin -
                   PongGame::kPaddleWidth,
               static_cast<int16_t>(game->paddleRight));

    display.fillRect(static_cast<int16_t>(game->ballX),
                     static_cast<int16_t>(game->ballY), PongGame::kBallSize,
                     PongGame::kBallSize, SSD1306_WHITE);

    display.display();
}

static void renderGameOver(const PongGame* game) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);

    const char* winner =
        (game->scoreLeft >= PONG_SCORE_TO_WIN) ? "HOST" : "GUEST";
    display.setCursor(10, 0);
    display.print(winner);
    display.setCursor(10, 20);
    display.print("WINS!");

    display.setTextSize(1);
    char scoreLine[16];
    snprintf(scoreLine, sizeof(scoreLine), "%u - %u", game->scoreLeft,
             game->scoreRight);
    display.setCursor(50, 40);
    display.print(scoreLine);

    display.setCursor(2, 50);
    display.print("Press button for menu");

    display.display();
}

bool pongUITickInput(PongGame* game, int8_t dirY, bool btnDown) {
    if (game == nullptr) {
        return false;
    }

    if (gRoleSelectionActive) {
        bool changed = false;
        if (dirY != 0) {
            gRoleChoice = (gRoleChoice == PongRoleChoice::Host)
                              ? PongRoleChoice::Join
                              : PongRoleChoice::Host;
            changed = true;
        }

        if (btnDown) {
            game->isHost = (gRoleChoice == PongRoleChoice::Host);
            pongSetup(game);
            gRoleSelectionActive = false;
            changed = true;
        }

        return changed;
    }

    if (game->gameOver) {
        if (btnDown) {
            gRoleSelectionActive = true;
            game->isRunning = false;
            game->gameOver = false;
            return true;
        }
        return false;
    }

    pongLoop(game, inputDirectionYContinuous());
    return true;
}

void pongUIInitDisplay(PongGame* game) {
    if (game == nullptr) {
        return;
    }

    game->isHost = true;
    initializeGame(game);
    game->isRunning = false;
    game->gameOver = false;

    gRoleChoice = PongRoleChoice::Host;
    gRoleSelectionActive = true;
    renderRoleSelect();
}

void pongUIUpdate(PongGame* game, PongUpdateKind kind) {
    if (game == nullptr) {
        return;
    }

    if (kind == PongAutoScreen) {
        if (gRoleSelectionActive) {
            kind = PongRoleSelectScreen;
        } else if (game->gameOver) {
            kind = PongGameOverScreen;
        } else {
            kind = PongMatchScreen;
        }
    }

    if (kind == PongRoleSelectScreen) {
        renderRoleSelect();
        return;
    }

    if (kind == PongGameOverScreen) {
        renderGameOver(game);
        return;
    }

    renderMatch(game);
}
