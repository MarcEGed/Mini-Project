#include <config.h>
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
}
