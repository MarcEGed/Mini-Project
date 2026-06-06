#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <config.h>
#include <stdint.h>

struct __attribute__((packed)) ChatMessage {
    uint8_t senderId;             // ASCII character 'A', 'B', etc.
    uint16_t timestampMs;         // millis() at send time (16-bit: relative only)
    char text[MSG_MAX_TEXT + 1];  // max text + '\0'
};

// CHAT is sent CRC + ARQ: the FEC layer prepends a 2-byte seq, so the app
// payload must fit the 24-byte ARQ region (FEC_ARQ_PAYLOAD_SIZE). 1+2+19 = 22.
static_assert(sizeof(ChatMessage) <= 24,
              "ChatMessage exceeds the 24-byte ARQ payload limit");

#endif // CHAT_MESSAGE_H
