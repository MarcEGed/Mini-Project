#include <config.h>
#include <debug.h>
#include <display.h>
#include <encryption.h>
#include <input.h>
#include <string.h>
#include <ftimers.h>
#include <selected_destination.h>
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
#include "ui/SessionTargetUI.h"
#include "ui/SyncTestUI.h"
#include "ui/ui.h"

transceiver xcvr;
ChatHandler chat;
PongGame pong;
ui appUI;

uint8_t SELECTED_DST_NODE = BROADCAST_DST_NODE;
static bool gTargetUIActive = false;
static UIMode gPendingMode = UIMode::Chat;

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

    if (gTargetUIActive) {
        if (backDown) {
            gTargetUIActive = false;
            appUI.setMode(UIMode::Menu);
            return;
        }

        bool confirmed = false;
        if (sessionTargetUITickInput(xcvr, dir, btnDown, SELECTED_DST_NODE,
                                     &SELECTED_DST_NODE, &confirmed)) {
            sessionTargetUIUpdate(xcvr);
        }

        if (confirmed) {
            gTargetUIActive = false;
            appUI.setMode(gPendingMode);
        }
        return;
    }

    if (backDown && appUI.mode != UIMode::Menu) {
        gTargetUIActive = false;
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
                    gPendingMode = UIMode::Pong;
                    gTargetUIActive = true;
                    sessionTargetUIInitDisplay(xcvr, SELECTED_DST_NODE);
                } else if (selection == MenuChat) {
                    gPendingMode = UIMode::Chat;
                    gTargetUIActive = true;
                    sessionTargetUIInitDisplay(xcvr, SELECTED_DST_NODE);
                } else if (selection == MenuSyncTest) {
                    gTargetUIActive = false;
                    appUI.setMode(UIMode::SyncTest);
                } else if (selection == MenuAudio) {
                    gPendingMode = UIMode::Audio;
                    gTargetUIActive = true;
                    sessionTargetUIInitDisplay(xcvr, SELECTED_DST_NODE);
                } else if (selection == MenuAbout) {
                    gTargetUIActive = false;
                    appUI.setMode(UIMode::About);
                } else {
                    gTargetUIActive = false;
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

                bool ok = selectedDestinationIsBroadcast()
                              ? xcvr.write(PacketType::CHAT, msg,
                                           SELECTED_DST_NODE)
                              : xcvr.writeReliable(PacketType::CHAT, msg,
                                                   SELECTED_DST_NODE);

                (void)ok;

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
#if AUDIO_SERIAL_DEBUG
            // Serial printing stalls the cooperative audio loop, so keep this
            // compile-time disabled except while diagnosing the audio path.
            {
                static uint32_t lastAudioDbg = 0;
                if (millis() - lastAudioDbg >= 2000) {
                    lastAudioDbg = millis();
                    audio::Stats s = audio::stats();
                    Serial.printf(
                        "\nAUDIO %s tx=%lu rx=%lu i2s=%lu/%lu mic=%lu "
                        "peak=%ld clip=%lu/%lu ring=%u under=%lu over=%lu | ",
                        audio::isTalking() ? "TALK" : "LISTEN",
                        (unsigned long)s.txPayloads,
                        (unsigned long)s.rxPayloads,
                        (unsigned long)(s.i2sReads - s.i2sEmptyReads),
                        (unsigned long)s.i2sReads,
                        (unsigned long)s.micSamples,
                        (long)s.micPeak,
                        (unsigned long)s.micClippedLow,
                        (unsigned long)s.micClippedHigh,
                        s.rxLevel,
                        (unsigned long)s.rxUnderruns,
                        (unsigned long)s.rxOverruns);
                    xcvr.logFecStats();
                }
            }
#endif
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
