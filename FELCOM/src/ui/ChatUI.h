#ifndef CHATUI_H
#define CHATUI_H

#include <stdint.h>

#include "chat/chat.h"
#include "chat/chatInput.h"
#include "chat/chatLog.h"

enum ChatUpdateKind : uint8_t {
    ChatMainScreen = 0,
    ChatInputLine = 1,
};

void chatUIInitDisplay(ChatHandler* chat);
void chatUIUpdate(ChatHandler* chat, ChatUpdateKind kind);

#endif
