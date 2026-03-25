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

transceiver xcvr;
chatLog msgLog;
chatInput msgInput;

void setup() {
    loggerSetup();
    // LOG_INFO("Booting node %c (id=0x%02X)", NODE_NAME, SENDER_ID);

    setupDisplay();
    // LOG_INFO("Display ready");

    inputInit();

    xcvr.setup();
    xcvr.setMode(RECEIVE);
    // LOG_INFO("Transceiver ready, listening on channel %d", xcvr.channel);

    msgLog.init();
    msgInput.init();

    // LOG_INFO("Boot complete");
}

void loop() {
    int8_t dir = inputDirectionY();
    bool btnDown = inputButtonPressed();

    msgInput.tickJoystick(dir, btnDown);

    // send
    if (msgInput.hasMessage()) {
        Message msg;
        msgInput.popMessage(msg, SENDER_ID, NODE_NAME);
        msgLog.push(msg);

        encrypt(msg.text, sizeof(msg.text));

        xcvr.setMode(TRANSMIT);
        bool ok = xcvr.write(&msg, sizeof(Message));
        xcvr.setMode(RECEIVE);

        if (ok)
            LOG_INFO("Sent: \"%s\"", msg.text);
        else
            LOG_ERROR("Send failed");
    }

    // receive — one read, one push
    Message incoming;
    if (xcvr.read(&incoming, sizeof(Message))) {
        decrypt(incoming.text, sizeof(incoming.text));

        LOG_INFO("Packet from 0x%02X %c: %s", incoming.senderId,
                 incoming.senderName, incoming.text);
        if (incoming.senderId != SENDER_ID) {
            msgLog.push(incoming);
        }
    }

    renderChat(msgLog, msgInput);
    delay(30);
}
