#include "ui.h"

#include "AboutUI.h"
#include "ChatUI.h"
#include "MenuUI.h"
#include "PongUI.h"
#include "RFTestUI.h"

void ui::init(ChatHandler* chatHandler, PongGame* pong, transceiver* xcvr) {
    this->chat = chatHandler;
    this->pongGame = pong;
    this->xcvr = xcvr;
    setMode(mode);
}

void ui::setMode(UIMode newMode) {
    mode = newMode;

    if (mode == UIMode::Menu) {
        menuUIInitDisplay();
        return;
    }

    if (mode == UIMode::Chat) {
        chatUIInitDisplay(chat, xcvr);
        return;
    }

    if (mode == UIMode::Pong) {
        pongUIInitDisplay(pongGame);
        return;
    }

    if (mode == UIMode::About) {
        aboutUIInitDisplay();
        return;
    }

    rfTestUIInitDisplay();
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
                (detail == static_cast<uint8_t>(ChatInputLine) ||
                 detail == static_cast<uint8_t>(ChatStatusBar))) {
                kind = static_cast<ChatUpdateKind>(detail);
            }
            chatUIUpdate(chat, kind, xcvr);

            break;
        }
        case UIMode::Pong: {
            pongUIUpdate(pongGame, PongAutoScreen);
            break;
        }
        case UIMode::RFTest: {
            rfTestUIUpdate(RFTestAutoScreen);
            break;
        }
        case UIMode::About: {
            aboutUIUpdate();
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

void ui::onPongStateChanged() {
    if (mode != UIMode::Pong) {
        return;
    }

    update(UIUpdateType::Full, static_cast<uint8_t>(PongAutoScreen));
}

void ui::onRFTestStateChanged() {
    if (mode != UIMode::RFTest) {
        return;
    }

    update(UIUpdateType::Full, static_cast<uint8_t>(RFTestAutoScreen));
}