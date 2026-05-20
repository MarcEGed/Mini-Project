#ifndef PONG_MESSAGE_H
#define PONG_MESSAGE_H

#include <stdint.h>

struct __attribute__((packed)) PongPacket {
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

static_assert(sizeof(PongPacket) <= 28,
              "PongPacket exceeds 28-byte FHSS payload limit");

#endif  // PONG_MESSAGE_H
