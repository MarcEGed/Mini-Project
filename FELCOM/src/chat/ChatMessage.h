#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <config.h>
#include <stdint.h>

struct __attribute__((packed)) ChatMessage {
    uint8_t senderId;             // ASCII character 'A', 'B', etc.
    uint32_t timestampMs;         // millis() at send time
    char text[MSG_MAX_TEXT + 1];  // max text + '\0'
};

static_assert(sizeof(ChatMessage) <= 28, "ChatMessage exceeds 28-byte FHSS payload limit");

#endif // CHAT_MESSAGE_H
