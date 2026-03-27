#include <config.h>
#include <debug.h>
#include <string.h>

#include "chat/chat.h"
#include "display.h"
#include "encryption.h"
#include "input.h"
#include "message.h"
#include "pong/pong.h"
#include "transceiver.h"
#include "ui/MenuUI.h"
#include "ui/ui.h"

transceiver xcvr;
ChatHandler chat;
PongGame pong;
ui appUI;

void setup() {
    loggerSetup();
    //LOG_INFO("Booting node %c (id=0x%02X)", NODE_NAME, SENDER_ID);

    setupDisplay();
    //LOG_INFO("Display ready");

    inputInit();

    xcvr.setup();
    xcvr.setMode(RECEIVE);
    LOG_INFO("Transceiver ready");

    chat.init();
    initializeGame(&pong);
    appUI.init(&chat, &pong);
    // LOG_INFO("Boot complete");
}

// void encoderIRQ(int dir, bool btn) {
//     msgInput.tickJoystick(dir, btn);
// }

void loop() {
    // LOG_INFO(xcvr.radio->testRPD() ? "Strong signal \> -64dBm on channel %d"
    //                                : "Weak signal \< -64dBm on channel %d",
    //          xcvr.radio->getChannel());
    int8_t dir = inputDirectionY();
    bool btnDown = inputButtonPressed();

    switch (appUI.mode) {
        case UIMode::Menu:
            if (menuUIUpdateSelection(dir)) {
                appUI.onMenuSelectionChanged();
            }
            if (btnDown) {
                appUI.setMode(menuUIGetSelection() == MenuPong ? UIMode::Pong
                                                               : UIMode::Chat);
            }
            break;
        case UIMode::Chat:
            if (chat.input.tickJoystick(dir, btnDown)) {
                appUI.onChatInputChanged();
            }

            // send
            if (chat.input.hasMessage()) {
                Message msg;
                chat.input.popMessage(msg, SENDER_ID, NODE_NAME);
                chat.log.push(msg);

                encrypt(msg.text, sizeof(msg.text));

                xcvr.setMode(TRANSMIT);
                bool ok = xcvr.write(&msg, sizeof(Message));
                xcvr.setMode(RECEIVE);

                if (ok)
                    LOG_INFO("Sent: \"%s\"", msg.text);
                else
                    LOG_ERROR("Send failed");

                appUI.onChatMessageAdded();
            }

            // receive — one read, one push
            Message incoming;
            if (xcvr.read(&incoming, sizeof(Message))) {
                decrypt(incoming.text, sizeof(incoming.text));

                LOG_INFO("Packet from 0x%02X %c: %s", incoming.senderId,
                         incoming.senderName, incoming.text);
                if (incoming.senderId != SENDER_ID) {
                    chat.log.push(incoming);
                    appUI.onChatMessageAdded();
                }
            }

            break;
        case UIMode::Pong:
            return;
        default:
            return;
    }

    // Keep this empty unless active section needs periodic redraws.
    delay(30);
}


//===================================
//===============BER MODE++++++++++++
//==================================
/*#include <config.h>
#include <debug.h>

#include "chat/chatInput.h"
#include "chat/chatLog.h"
#include "chat/chatRenderer.h"
#include "display.h"
#include "encryption.h"
#include "input.h"
#include "message.h"
#include "pong/pong.h"
#include "transceiver.h"
#include "test/testMode.h"

transceiver xcvr;

void setup(){
    xcvr.setup();
    xcvr.setMode(RECEIVE);
    setupDisplay();
    rfTestSetup(xcvr);
}

void loop(){
    rfTestLoop(xcvr);
}*/


//==================================
//this is the main.cpp that made texting work
//==================================

/*#include <config.h>
#include <debug.h>

#include "display.h"
#include "input.h"
#include "message.h"
#include "chat\chatInput.h"
#include "chat\chatLog.h"
#include "chat\chatRenderer.h"
#include "transceiver.h"

#include "pong/pong.h"

transceiver xcvr;
chatLog     Log;
chatInput   Input;

void setup() {
    loggerSetup();
    //LOG_INFO("Booting node %c (id=0x%02X)", NODE_NAME, SENDER_ID);

    setupDisplay();
    //LOG_INFO("Display ready");

    inputInit();

    xcvr.setup();
    xcvr.setMode(RECEIVE);
    //LOG_INFO("Transceiver ready, listening on channel %d", xcvr.channel);

    Log.init();
    Input.init();

    //LOG_INFO("Boot complete");
}

void loop() {
    int8_t dir     = inputDirectionY();
    bool   btnDown = inputButtonPressed();

    Input.tickJoystick(dir, btnDown);

    // send
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

    // receive — one read, one push
    Message incoming;
    if (xcvr.read(&incoming, sizeof(Message))) {
        LOG_INFO("Packet from 0x%02X %c: %s", incoming.senderId, incoming.senderName, incoming.text);
        if (incoming.senderId != SENDER_ID) {
            Log.push(incoming);
        }
    }

    renderChat(Log, Input);
    delay(30);
}*/