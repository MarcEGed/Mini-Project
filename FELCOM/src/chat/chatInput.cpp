#include "chatInput.h"
#include <config.h>
#include <string.h>

static const char CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?'-:)\x7F";
static const uint8_t CHARSET_LEN = sizeof(CHARSET) - 1;
static const char SEND_CHAR = '\x7F'; //confirms sending

void chatInput::init(){
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    currentChar = CHARSET[0];
    messageReady = false;
    lastMoveMs = 0;
    lastDirY = 0;
}

void chatInput::tickJoystick(int8_t dirY, bool btnPressed){
    if (dirY != 0 && lastDirY == 0){
        advance(dirY);
    }
    lastDirY = dirY;
    if (btnPressed){
        commit();
    }
}

void chatInput::advance(int8_t dir){
    uint8_t index = 0;
    for (uint8_t i = 0; i < CHARSET_LEN; i++){
        if (CHARSET[i] == currentChar){
            index = i;
            break;
        }
    }
    index = (index + dir + CHARSET_LEN) % CHARSET_LEN;
    currentChar = CHARSET[index];
}

void chatInput::commit(){
    // Send character -> transmits whatever is in the buffer
    if (currentChar == SEND_CHAR) {
        if (length > 0) {
            messageReady = true;
        }
        return; //don't append shit
    }

    //add character character if there's room
    if (length < MAX_INPUT_LENGTH) {
        buffer[length++] = currentChar;
        buffer[length]   = '\0';
    }

    //auto-send if buffer is completely full
    if (length >= MAX_INPUT_LENGTH) {
        messageReady = true;
    }
}

bool chatInput::hasMessage(){
    return messageReady;
}

void chatInput::popMessage(Message& out, uint8_t senderID, char name){
    out.senderId = senderID;
    out.senderName = name;
    out.timestampMs = millis();
    strncpy(out.text, buffer, MSG_MAX_TEXT);
    out.text[MSG_MAX_TEXT - 1] = '\0';

    //reset
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    currentChar = CHARSET[0];
    messageReady = false;
}

