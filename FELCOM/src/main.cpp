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
#include "test/audioTest.h"
#include "test/buttonToneTest.h"

transceiver xcvr;

void setup(){
    xcvr.setup();
    xcvr.setMode(RECEIVE);
    setupDisplay();
    
    // Uncomment to run RF test
    // rfTestSetup(xcvr);
    
    // Run audio test with microphone streaming
    //audioTestSetup();
    
    // Uncomment to run button tone test
    buttonToneTestSetup();
}

void loop(){
    // Uncomment to run RF test
    // rfTestLoop(xcvr);
    
    // Run audio test
    audioTestLoop();
    
    // Uncomment to run button tone test
    //buttonToneTestLoop();
}
