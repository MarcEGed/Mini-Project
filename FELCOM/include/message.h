#ifndef MESSAGE_H
#define MESSAGE_H

#include <config.h>
#include <stdint.h>
struct __attribute__((packed)) Message{
    uint8_t  senderId;       // defined in config
    char     senderName;  // A, B, etc
    uint32_t timestampMs;    // millis() at send time
    char     text[MSG_MAX_TEXT + 1]; //max 25 chars + '\0'
};

static_assert(sizeof(Message) <= 32, "Message exceeds RF24 max payload");

#endif