#include <config.h>
#include <debug.h>
#include <display.h>
#include <encryption.h>
#include <input.h>
#include <string.h>
#include <ftimers.h>
#include <transceiver.h>

#include "chat/ChatMessage.h"
#include "chat/chat.h"
#include "pong/pong.h"
#include "rf_test/testMode.h"
#include "ui/MenuUI.h"
#include "ui/PongUI.h"
#include "ui/RFTestUI.h"
#include "ui/SyncTestUI.h"
#include "ui/ui.h"

transceiver xcvr;
ChatHandler chat;
PongGame pong;
ui appUI;

void setup() {
    loggerSetup();
    // LOG_INFO("Booting node (id=0x%02X)", NODE_ID);

    setupDisplay();
    // LOG_INFO("Display ready");

    inputInit();

    initCounter();

    xcvr.setup();
    xcvr.setChannel(HOPPING_CHANNELS[readCounter() % HOPPING_CHANNELS_SIZE]);
    xcvr.setMode(RECEIVE);
    LOG_INFO("Transceiver ready");

    chat.init();
    initializeGame(&pong);
    appUI.init(&chat, &pong);
    // LOG_INFO("Boot complete");
}

void loop() {
    // LOG_INFO(xcvr.radio->testRPD() ? "Strong signal \> -64dBm on channel %d"
    //                                : "Weak signal \< -64dBm on channel %d",
    //          xcvr.radio->getChannel());

    static uint32_t lastHop = 0;
    uint32_t currentHop = readCounter() % HOPPING_CHANNELS_SIZE;
    if (currentHop != lastHop) {
        lastHop = currentHop;
        xcvr.setChannel(HOPPING_CHANNELS[currentHop]);
        xcvr.setMode(RECEIVE);
    }


    int8_t dir = inputDirectionY();
    bool btnDown = inputButtonPressed();
    bool backDown = inputBackPressed();

    if (backDown && appUI.mode != UIMode::Menu) {
        appUI.setMode(UIMode::Menu);
    }

    switch (appUI.mode) {
        case UIMode::Menu:
            if (menuUIUpdateSelection(dir)) {
                appUI.onMenuSelectionChanged();
            }
            if (btnDown) {
                MenuModeSelection selection = menuUIGetSelection();
                if (selection == MenuPong) {
                    appUI.setMode(UIMode::Pong);
                } else if (selection == MenuChat) {
                    appUI.setMode(UIMode::Chat);
                } else if (selection == MenuSyncTest) {
                    appUI.setMode(UIMode::SyncTest);
                } else if (selection == MenuAbout) {
                    appUI.setMode(UIMode::About);
                } else {
                    appUI.setMode(UIMode::RFTest);
                }
            }
            break;
        case UIMode::Chat:
            if (chat.input.tickJoystick(dir, btnDown)) {
                appUI.onChatInputChanged();
            }

            // send
            if (chat.input.hasMessage()) {
                ChatMessage msg;
                chat.input.popMessage(msg, NODE_ID);
                chat.log.push(msg);

                encrypt(msg.text, sizeof(msg.text));

                bool ok = xcvr.write(PacketType::CHAT, msg);

                if (ok)
                    LOG_INFO("Sent: \"%s\"", msg.text);
                else
                    LOG_ERROR("Send failed");

                appUI.onChatMessageAdded();
            }

            // receive — one read, one push
            ChatMessage incoming;
            if (xcvr.read(PacketType::CHAT, incoming)) {
                decrypt(incoming.text, sizeof(incoming.text));

                LOG_INFO("Packet from 0x%02X: %s", incoming.senderId,
                         incoming.text);
                chat.log.push(incoming);
                appUI.onChatMessageAdded();
            }

            break;
        case UIMode::Pong:
            if (pongUITickInput(&pong, dir, btnDown)) {
                appUI.onPongStateChanged();
            }
            break;
        case UIMode::RFTest:
            if (rfTestUITickInput(xcvr, dir, btnDown, millis())) {
                appUI.onRFTestStateChanged();
            }
            break;
        case UIMode::SyncTest:
            if (syncTestUITickInput(xcvr, dir, btnDown, millis(),
                                    readCounter())) {
                appUI.onSyncTestStateChanged();
            }
            break;
        case UIMode::About:
            // About has no active input right now besides back button
            break;
        default:
            return;
    }

    // Keep this empty unless active section needs periodic redraws.
    // delay(30);
}
