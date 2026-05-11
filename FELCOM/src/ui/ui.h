#ifndef UIHANDLER_H
#define UIHANDLER_H

#include <stdint.h>

#include "chat/chat.h"
#include "pong/pong.h"

enum UIMode : uint8_t {
    Menu = 0,
    Chat = 1,
    Pong = 2,
    RFTest = 3,
    Audio = 4,
    About = 5,
};

enum UIUpdateType : uint8_t {
    Full = 0,
    Incremental = 1,
};

struct ui {
    UIMode mode = UIMode::Menu;

    PongGame* pongGame = nullptr;
    ChatHandler* chat = nullptr;

    void init(ChatHandler* chatHandler, PongGame* pong);
    void setMode(UIMode newMode);
    void update(UIUpdateType domain, uint8_t detail = 0);
    void onChatInputChanged();
    void onChatMessageAdded();
    void onMenuSelectionChanged();
    void onPongStateChanged();
    void onRFTestStateChanged();
    void onAudioStateChanged();
};

#endif
