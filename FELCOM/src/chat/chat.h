#ifndef CHAT_H
#define CHAT_H

#include "chatInput.h"
#include "chatLog.h"

struct ChatHandler {
    chatLog log;
    chatInput input;

    void init() {
        log.init();
        input.init();
    }
};

#endif  // CHAT_H