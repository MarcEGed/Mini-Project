#ifndef CHATLOG_H
#define CHATLOG_H
#include <config.h>
#include <message.h>

// chatLog handles data like a ring buffer
// not for manyake purposes, but ig it's more efficient
// TLDR: oldest data gets replaced by newest when full
struct chatLog {
    Message entries[LOG_SIZE];
    uint8_t count;
    uint8_t head;

    void init();
    void push(Message& msg);
    Message* get(uint8_t index);
    uint8_t size();
};

#endif