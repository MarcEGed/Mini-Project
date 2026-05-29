#include <config.h>
#include <debug.h>
#include <display.h>
#include <encryption.h>
#include <input.h>
#include <string.h>
#include <transceiver.h>

#include "chat/chat.h"
#include "chat/ChatMessage.h"
#include "pong/pong.h"
#include "rf_test/testMode.h"
#include "ui/MenuUI.h"
#include "ui/ChatUI.h"
#include "ui/PongUI.h"
#include "ui/RFTestUI.h"
#include "ui/ui.h"

transceiver xcvr;
ChatHandler chat;
PongGame pong;
ui appUI;

hw_timer_t* fhss_timer = NULL;

// ISR only sets a flag; actual RF24 operations happen in main loop
void IRAM_ATTR onFhssTimer() { xcvr.needs_hop = true; }

void setup() {
    loggerSetup();
    // LOG_INFO("Booting node (id=0x%02X)", NODE_ID);

    setupDisplay();
    // LOG_INFO("Display ready");

    inputInit();

    xcvr.setup();
    xcvr.setMode(RECEIVE);
    LOG_INFO("Transceiver ready");

    // Setup FHSS Timer: 2ms interval
    fhss_timer = timerBegin(1, 80, true);
    timerAttachInterrupt(fhss_timer, &onFhssTimer, true);
    timerAlarmWrite(fhss_timer, FHSS_TIMER_PERIOD_US, true);
    // Don't enable it yet; it's enabled in loop() once we've joined
    xcvr.fhss_timer = fhss_timer;

    LOG_INFO("Starting active scan join...");
    xcvr.startActiveScan(millis());

    chat.init();
    initializeGame(&pong);
    appUI.init(&chat, &pong, &xcvr);
    // LOG_INFO("Boot complete");
}

// void encoderIRQ(int dir, bool btn) {
//     msgInput.tickJoystick(dir, btn);
// }

void loop() {
    // Check if FHSS timer requested a channel hop
    if (xcvr.needs_hop) {
        xcvr.needs_hop = false;
        xcvr.hop();
    }

    // LOG_INFO(xcvr.radio->testRPD() ? "Strong signal \> -64dBm on channel %d"
    //                                : "Weak signal \< -64dBm on channel %d",
    //          xcvr.radio->getChannel());
    xcvr.updateJoin(millis());
    
    // Enable timer once we've joined the network
    static bool timer_enabled = false;
    if (!xcvr.joined && timer_enabled) {
        timer_enabled = false;
    }
    if (xcvr.joined && !timer_enabled) {
        timerAlarmEnable(fhss_timer);
        timer_enabled = true;
        LOG_INFO("FHSS timer enabled");
    }

    int8_t dir = inputDirectionY();
    bool btnDown = inputButtonPressed();
    bool backDown = inputBackPressed();

    static uint32_t lastChatStatusMs = 0;
    if (appUI.mode == UIMode::Chat) {
        const uint32_t now = millis();
        if (now - lastChatStatusMs >= 250) {
            lastChatStatusMs = now;
            appUI.update(UIUpdateType::Incremental,
                         static_cast<uint8_t>(ChatStatusBar));
        }
    }

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
        case UIMode::About:
            // About has no active input right now besides back button
            break;
        default:
            return;
    }

    // Keep this empty unless active section needs periodic redraws.
    // delay(30);
}
