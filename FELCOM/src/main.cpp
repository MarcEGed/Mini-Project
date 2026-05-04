#include <config.h>
#include <debug.h>

#include "display.h"
#include "input.h"
#include "transceiver.h"
#include "chat/chatLog.h"
#include "chat/chatInput.h"
#include "menu/menu.h"
#include "pong/pong.h"
#include "audio.h"

transceiver  xcvr;
chatLog      Log;
chatInput    Input;
AudioHandler audio(&xcvr);

void setup() {
    loggerSetup();
    setupDisplay();
    inputInit();
    xcvr.setup();
    menuSetup();
    audio.begin();
}

void loop() {
    menuLoop();

    #if AUDIO_ENABLED == 1
        audio.txTick();
    #elif AUDIO_ENABLED == 2
        audio.rxTick();
    #endif
}