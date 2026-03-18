#ifndef PONG_H
#define PONG_H

struct GamePacket
{
    int ballX;
    int ballY;
    int paddleLeft;
    int paddleRight;
};

void drawGame(GamePacket *game);
#endif // PONG_H