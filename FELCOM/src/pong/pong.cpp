#include "pong.h"
#include "display.h"
#include "../input/input.h"
#include "../transceiver/transceiver.h"
#include <config.h>
#include <debug.h>

#define SCREEN_W        128
#define SCREEN_H        64
#define PADDLE_W        3
#define PADDLE_H        12
#define BALL_SIZE       3
#define PADDLE_MARGIN   4
#define SCORE_TO_WIN    7
#define TICK_MS         30

#define PONG_PKT_STATE  0x01
#define PONG_PKT_INPUT  0x02

struct PongPacket {
    uint8_t  type;
    uint8_t  reserved[3];
    int16_t  paddleY;      // sender's paddle Y
    int16_t  ballX;        // only meaningful from host
    int16_t  ballY;        // only meaningful from host
    int8_t   ballVX;
    int8_t   ballVY;
    uint8_t  scoreHost;
    uint8_t  scoreGuest;
};


//host runs physics, guest sends input and mirrors state
#define IS_HOST  RF_TEST_TX   //reuse TX/RX compile flag

static float  paddleHostY;
static float  paddleGuestY;
static float  ballX, ballY;
static float  ballVX, ballVY;
static uint8_t scoreHost  = 0;
static uint8_t scoreGuest = 0;

static uint32_t lastTickMs  = 0;
static uint32_t lastTxMs    = 0;
#define TX_INTERVAL_MS  15

static bool gameOver = false;

static void resetBall(bool toHost) {
    ballX  = SCREEN_W / 2.0f;
    ballY  = SCREEN_H / 2.0f;
    ballVX = toHost ? -2.5f : 2.5f;
    ballVY = 1.5f;
}

static void clampPaddle(float& y) {
    if (y < 0)                        y = 0;
    if (y > SCREEN_H - PADDLE_H)      y = SCREEN_H - PADDLE_H;
}

static void renderPong() {
    display.clearDisplay();

    //scores
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    char sc[8];
    snprintf(sc, sizeof(sc), "%u  %u", scoreHost, scoreGuest);
    display.setCursor(SCREEN_W / 2 - 12, 0);
    display.print(sc);

    //centre dashes
    for (uint8_t y = 8; y < SCREEN_H; y += 6)
        display.drawPixel(SCREEN_W / 2, y, SSD1306_WHITE);

    //paddles
    display.fillRect(PADDLE_MARGIN,
                     (int16_t)paddleHostY,
                     PADDLE_W, PADDLE_H, SSD1306_WHITE);
    display.fillRect(SCREEN_W - PADDLE_MARGIN - PADDLE_W,
                     (int16_t)paddleGuestY,
                     PADDLE_W, PADDLE_H, SSD1306_WHITE);

    //ball
    display.fillRect((int16_t)ballX, (int16_t)ballY,
                     BALL_SIZE, BALL_SIZE, SSD1306_WHITE);

    display.display();
}

static void renderGameOver() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);

    const char* winner = (scoreHost >= SCORE_TO_WIN) ? "HOST" : "GUEST";
    display.setCursor(10, 10);
    display.print(winner);
    display.setCursor(10, 30);
    display.print("WINS!");

    display.setTextSize(1);
    char sc[16];
    snprintf(sc, sizeof(sc), "%u - %u", scoreHost, scoreGuest);
    display.setCursor(50, 52);
    display.print(sc);

    display.display();
}

//network

extern transceiver xcvr;

static void sendState(float myPaddleY) {
    PongPacket pkt{};
#if IS_HOST
    pkt.type      = PONG_PKT_STATE;
    pkt.paddleY   = (int16_t)paddleHostY;
    pkt.ballX     = (int16_t)ballX;
    pkt.ballY     = (int16_t)ballY;
    pkt.ballVX    = (int8_t)ballVX;
    pkt.ballVY    = (int8_t)ballVY;
    pkt.scoreHost  = scoreHost;
    pkt.scoreGuest = scoreGuest;
#else
    pkt.type    = PONG_PKT_INPUT;
    pkt.paddleY = (int16_t)myPaddleY;
#endif
    xcvr.setMode(TRANSMIT);
    xcvr.write(&pkt, sizeof(PongPacket));
    xcvr.setMode(RECEIVE);
}

static void receivePackets() {
    PongPacket pkt{};
    for (uint8_t i = 0; i < 4; i++) {
        if (!xcvr.read(&pkt, sizeof(PongPacket))) break;

#if IS_HOST
        if (pkt.type == PONG_PKT_INPUT)
            paddleGuestY = pkt.paddleY;
#else
        if (pkt.type == PONG_PKT_STATE) {
            paddleHostY  = pkt.paddleY;
            ballX        = pkt.ballX;
            ballY        = pkt.ballY;
            ballVX       = pkt.ballVX;
            ballVY       = pkt.ballVY;
            scoreHost    = pkt.scoreHost;
            scoreGuest   = pkt.scoreGuest;
            if (scoreHost >= SCORE_TO_WIN || scoreGuest >= SCORE_TO_WIN)
                gameOver = true;
        }
#endif
    }
}

//physics (host only)

static void tickPhysics() {
    ballX += ballVX;
    ballY += ballVY;

    //top / bottom bounce
    if (ballY <= 0)                        { ballY  = 0;                ballVY = -ballVY; }
    if (ballY >= SCREEN_H - BALL_SIZE)     { ballY  = SCREEN_H - BALL_SIZE; ballVY = -ballVY; }

    //host paddle
    if (ballVX < 0 &&
        ballX <= PADDLE_MARGIN + PADDLE_W &&
        ballY + BALL_SIZE >= paddleHostY &&
        ballY <= paddleHostY + PADDLE_H) {
        ballVX = -ballVX * 1.05f;   // slight speedup on hit
        ballX  = PADDLE_MARGIN + PADDLE_W;
    }

    //guest paddle
    if (ballVX > 0 &&
        ballX + BALL_SIZE >= SCREEN_W - PADDLE_MARGIN - PADDLE_W &&
        ballY + BALL_SIZE >= paddleGuestY &&
        ballY <= paddleGuestY + PADDLE_H) {
        ballVX = -ballVX * 1.05f;
        ballX  = SCREEN_W - PADDLE_MARGIN - PADDLE_W - BALL_SIZE;
    }

    //scoring
    if (ballX < 0) {
        scoreGuest++;
        if (scoreGuest >= SCORE_TO_WIN) { gameOver = true; return; }
        resetBall(false);
    }
    if (ballX > SCREEN_W) {
        scoreHost++;
        if (scoreHost >= SCORE_TO_WIN) { gameOver = true; return; }
        resetBall(true);
    }
}

void pongSetup() {
    scoreHost  = 0;
    scoreGuest = 0;
    gameOver   = false;
    paddleHostY  = (SCREEN_H - PADDLE_H) / 2.0f;
    paddleGuestY = (SCREEN_H - PADDLE_H) / 2.0f;
    lastTickMs   = millis();
    lastTxMs     = millis();
    resetBall(true);
    xcvr.setMode(RECEIVE);
    LOG_INFO("Pong start, role=%s", IS_HOST ? "HOST" : "GUEST");
}

void pongLoop() {
    uint32_t now = millis();

    receivePackets();

    if (gameOver) {
        renderGameOver();
        return;
    }

    //move own paddle from joystick
    int8_t dir = inputDirectionY();
#if IS_HOST
    paddleHostY  += dir * 2.5f;
    clampPaddle(paddleHostY);
#else
    paddleGuestY += dir * 2.5f;
    clampPaddle(paddleGuestY);
#endif

#if IS_HOST
    if (now - lastTickMs >= TICK_MS) {
        lastTickMs = now;
        tickPhysics();
    }
#endif

    if (now - lastTxMs >= TX_INTERVAL_MS) {
        lastTxMs = now;
#if IS_HOST
        sendState(paddleHostY);
#else
        sendState(paddleGuestY);
#endif
    }

    renderPong();
}