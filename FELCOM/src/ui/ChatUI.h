#ifndef CHATUI_H
#define CHATUI_H

#include <stdint.h>

#include "chat/chat.h"
#include "chat/chatInput.h"
#include "chat/chatLog.h"

struct transceiver;

enum ChatUpdateKind : uint8_t {
    ChatMainScreen = 0,
    ChatInputLine = 1,
    ChatStatusBar = 2,
};

void chatUIInitDisplay(ChatHandler* chat, const transceiver* xcvr);
void chatUIUpdate(ChatHandler* chat, ChatUpdateKind kind,
                  const transceiver* xcvr);

#endif
