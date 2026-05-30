#include "chatInput.h"

#include <Arduino.h>
#include <config.h>
#include <string.h>

static const char CHARSET[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?'-:)\x7F";
static const uint8_t CHARSET_LEN = sizeof(CHARSET) - 1;
static const char SEND_CHAR = '\x7F';  // confirms sending

void chatInput::init() {
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    currentIndex = 0;
    messageReady = false;
    lastMoveMs = 0;
    lastDirY = 0;
}

bool chatInput::tickJoystick(int8_t dirY, bool btnPressed) {
    // return true if we should update the input display (moved or sent message)
    advance(dirY);
    if (btnPressed) {
        commit();
    }
    return dirY != 0 || btnPressed;
}

void chatInput::advance(int8_t dir) {
    currentIndex = (currentIndex + dir + CHARSET_LEN) % CHARSET_LEN;
}

void chatInput::commit() {
    // Send character -> transmits whatever is in the buffer
    if (CHARSET[currentIndex] == SEND_CHAR) {
        if (length > 0) {
            messageReady = true;
        }
        return;  // don't append shit
    }

    // add character character if there's room
    if (length < MAX_INPUT_LENGTH) {
        buffer[length++] = CHARSET[currentIndex];
        buffer[length] = '\0';
    }

    // auto-send if buffer is completely full
    if (length >= MAX_INPUT_LENGTH) {
        messageReady = true;
    }
}

bool chatInput::hasMessage() { return messageReady; }

char chatInput::selectedChar() const { return CHARSET[currentIndex]; }

void chatInput::popMessage(ChatMessage& out, uint8_t senderID) {
    out.senderId = senderID;
    out.timestampMs = millis();
    strncpy(out.text, buffer, MSG_MAX_TEXT);
    out.text[MSG_MAX_TEXT - 1] = '\0';

    // reset
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    currentIndex = 0;
    messageReady = false;
}
