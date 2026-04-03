#include "menu.h"
#include "display.h"
#include "input.h"
#include "transceiver.h"
#include "test/testMode.h"
#include "chat/chatInput.h"
#include "chat/chatLog.h"
#include "chat/chatRenderer.h"
#include "message.h"
#include "pong/pong.h"
#include <config.h>
#include <debug.h>

extern transceiver xcvr;
extern chatLog     Log;
extern chatInput   Input;

static appMode   currentMode   = MODE_MENU;
static uint8_t   menuSelection = 0;

static const char* menuItems[]  = { "Chat", "RF Test", "Pong"};
static const uint8_t MENU_COUNT = 3;

// ── menu ─────────────────────────────────────────────────────────────────────

static void renderMenu() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("== SELECT MODE ==");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    for (uint8_t i = 0; i < MENU_COUNT; i++) {
        if (i == menuSelection) {
            display.fillRect(0, 16 + i * 16, 128, 14, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }
        display.setTextSize(1);
        display.setCursor(10, 18 + i * 16);
        display.print(menuItems[i]);
    }

    display.setTextColor(SSD1306_WHITE);
    display.display();
}

static void tickMenu() {
    int8_t dir     = inputDirectionY();
    bool   btnDown = inputButtonPressed();

    if (dir > 0) menuSelection = (menuSelection + 1) % MENU_COUNT;
    if (dir < 0) menuSelection = (menuSelection - 1 + MENU_COUNT) % MENU_COUNT;

    if (btnDown) {
        if (menuSelection == 0) {
            currentMode = MODE_CHAT;
            Log.init();
            Input.init();
            xcvr.setMode(RECEIVE);
        } else if (menuSelection == 1) {
            currentMode = MODE_RFTEST;
            rfTestSetup(xcvr);
        } else if (menuSelection == 2) {
            currentMode = MODE_PONG;
            pongSetup();
        }
        return;
    }

    renderMenu();
}

// ── chat ─────────────────────────────────────────────────────────────────────

static void tickChat() {
    int8_t dir     = inputDirectionY();
    bool   btnDown = inputButtonPressed();

    Input.tickJoystick(dir, btnDown);

    if (Input.hasMessage()) {
        Message msg;
        Input.popMessage(msg, SENDER_ID, NODE_NAME);
        Log.push(msg);

        xcvr.setMode(TRANSMIT);
        bool ok = xcvr.write(&msg, sizeof(Message));
        xcvr.setMode(RECEIVE);

        if (ok) LOG_INFO("Sent: \"%s\"", msg.text);
        else    LOG_ERROR("Send failed");
    }

    Message incoming;
    if (xcvr.read(&incoming, sizeof(Message))) {
        if (incoming.senderId != SENDER_ID)
            Log.push(incoming);
    }

    renderChat(Log, Input);
}

// ── public API ───────────────────────────────────────────────────────────────

void menuSetup() {
    currentMode   = MODE_MENU;
    menuSelection = 0;
    renderMenu();
}

void menuLoop() {
    switch (currentMode) {
        case MODE_MENU:
            tickMenu();
            delay(150);
            break;
        case MODE_CHAT:
            tickChat();
            delay(30);
            break;
        case MODE_RFTEST:
            rfTestLoop(xcvr);
            delay(30);
            break;
        case MODE_PONG:
            pongLoop();
            delay(30);
            break;
    }
}