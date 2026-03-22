#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MSG_MAX_TEXT 25  //1 (id) + 1 (name[2]) + 4 (ts) + 25 (text) + 1 spare = 32

struct __attribute__((packed)) Message{
    uint8_t  senderId;       // defined in config
    char     senderName;  // A, B, etc
    uint32_t timestampMs;    // millis() at send time
    char     text[MSG_MAX_TEXT + 1]; //max 25 chars + '\0'
};

static_assert(sizeof(Message) <= 32, "Message exceeds RF24 max payload");

#endif