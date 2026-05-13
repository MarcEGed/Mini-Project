#ifndef MESSAGE_H
#define MESSAGE_H

#include <config.h>
#include <stdint.h>
struct __attribute__((packed)) Message {
    uint8_t senderId;             // ASCII character 'A', 'B', etc.
    uint32_t timestampMs;         // millis() at send time
    char text[MSG_MAX_TEXT + 1];  // max text + '\0'
};

static_assert(sizeof(Message) <= 28, "Message exceeds 28-byte FHSS payload limit");

#endif