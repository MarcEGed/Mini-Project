#include "ui.h"

#include "ChatUI.h"
#include "MenuUI.h"
#include "PongUI.h"

void ui::init(ChatHandler* chatHandler, PongGame* pong) {
    this->chat = chatHandler;
    this->pongGame = pong;
    setMode(mode);
}

void ui::setMode(UIMode newMode) {
    mode = newMode;

    if (mode == UIMode::Menu) {
        menuUIInitDisplay();
        return;
    }

    if (mode == UIMode::Chat) {
        chatUIInitDisplay(chat);
        return;
    }

    pongUIInitDisplay(pongGame);
}

void ui::update(UIUpdateType domain, uint8_t detail) {
    switch (mode) {
        case UIMode::Menu: {
            menuUIUpdate();
            break;
        }
        case UIMode::Chat: {
            ChatUpdateKind kind = ChatMainScreen;
            if (domain == UIUpdateType::Incremental &&
                detail == static_cast<uint8_t>(ChatInputLine)) {
                kind = ChatInputLine;
            }
            chatUIUpdate(chat, kind);

            break;
        }
        case UIMode::Pong: {
            pongUIUpdate(pongGame, PongMainScreen);
            break;
        }
        default:
            break;
    }
}

void ui::onChatInputChanged() {
    if (mode != UIMode::Chat) {
        return;
    }

    update(UIUpdateType::Incremental, static_cast<uint8_t>(ChatInputLine));
}

void ui::onChatMessageAdded() {
    if (mode != UIMode::Chat) {
        return;
    }

    update(UIUpdateType::Full, static_cast<uint8_t>(ChatMainScreen));
}

void ui::onMenuSelectionChanged() {
    if (mode != UIMode::Menu) {
        return;
    }

    update(UIUpdateType::Full);
}