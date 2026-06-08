#include <config.h>
#include <debug.h>
#include <display.h>
#include <encryption.h>
#include <input.h>
#include <string.h>
#include <ftimers.h>
#include <transceiver.h>

#include "audio.h"
#include "chat/ChatMessage.h"
#include "chat/chat.h"
#include "pong/pong.h"
#include "rf_test/testMode.h"
#include "ui/AudioUI.h"
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

#if defined(MIC_TEST_MODE) && MIC_TEST_MODE
    // Mic bring-up diagnostic: skip the normal app entirely and run a pure mic
    // monitor (serial span + slot-0 to the speaker). Set MIC_TEST_MODE 0 in
    // config.h for normal operation. Never returns.
    audio::micProbe();
#endif

    setupDisplay();
    // LOG_INFO("Display ready");

    inputInit();

    initCounter();

    xcvr.setup();
    xcvr.setChannel(HOPPING_CHANNELS[readCounter() % HOPPING_CHANNELS_SIZE]);
    xcvr.setMode(RECEIVE);
    LOG_INFO("Transceiver ready");

    audio::begin(&xcvr);  // I2S mic (stereo slot0) + DAC; idle until AUDIO screen

    chat.init();
    initializeGame(&pong);
    appUI.init(&chat, &pong);
    // LOG_INFO("Boot complete");
}

void loop() {
    static uint32_t lastHop = 0;
    uint32_t currentHop = readCounter() % HOPPING_CHANNELS_SIZE;
    if (currentHop != lastHop) {
        lastHop = currentHop;
        xcvr.setChannel(HOPPING_CHANNELS[currentHop]);
        xcvr.setMode(RECEIVE);
        // Skip the serial log during audio — flush() stalls ~50 ms and glitches
        // playback. The hop itself still happens.
        if (appUI.mode != UIMode::Audio) {
            LOG_INFO("Hopped to channel %d", HOPPING_CHANNELS[currentHop]);
        }
    }

    // Throttled FEC summary; suppressed in audio mode (a blocking serial flush
    // would interrupt real-time playback). The AUDIO screen has its own opt-in
    // debug line below.
    static uint32_t lastFecStatsMs = 0;
    if (appUI.mode != UIMode::Audio && millis() - lastFecStatsMs >= 5000) {
        lastFecStatsMs = millis();
        xcvr.logFecStats();
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
                } else if (selection == MenuAudio) {
                    appUI.setMode(UIMode::Audio);
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

                // CHAT is reliable: CRC + ARQ (retransmit until ACKed).
                bool ok = xcvr.writeReliable(PacketType::CHAT, msg);

                //if (ok)
                //    LOG_INFO("Sent: \"%s\"", msg.text);
                //else
                //    LOG_ERROR("Send failed");

                appUI.onChatMessageAdded();
            }

            // receive — one read, one push
            ChatMessage incoming;
            if (xcvr.read(PacketType::CHAT, incoming)) {
                decrypt(incoming.text, sizeof(incoming.text));

                //LOG_INFO("Packet from 0x%02X: %s", incoming.senderId,
                //         incoming.text);
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
        case UIMode::Audio:
            // Pumps capture/TX (talking) or RX/playback (listening) every loop.
            if (audioUITickInput(dir, btnDown)) {
                appUI.onAudioStateChanged();
            }
            // ── DEBUG: comment out this block for glitch-free audio ──────────
            // (the serial flush stalls playback ~50 ms each time it prints).
            {
                static uint32_t lastAudioDbg = 0;
                if (millis() - lastAudioDbg >= 2000) {
                    lastAudioDbg = millis();
                    Serial.printf("AUDIO %s tx=%lu rx=%lu | ",
                                  audio::isTalking() ? "TALK" : "LISTEN",
                                  (unsigned long)audio::txPayloads(),
                                  (unsigned long)audio::rxPayloads());
                    xcvr.logFecStats();
                }
            }
            // ─────────────────────────────────────────────────────────────────
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
