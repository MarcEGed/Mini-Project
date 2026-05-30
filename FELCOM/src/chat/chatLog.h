#ifndef CHATLOG_H
#define CHATLOG_H
#include <config.h>
#include "chat/ChatMessage.h"

// chatLog handles data like a ring buffer
// not for manyake purposes, but ig it's more efficient
// TLDR: oldest data gets replaced by newest when full
struct chatLog {
    ChatMessage entries[LOG_SIZE];
    uint8_t count;
    uint8_t head;

    void init();
    void push(ChatMessage& msg);
    ChatMessage* get(uint8_t index);
    uint8_t size();
};

#endif