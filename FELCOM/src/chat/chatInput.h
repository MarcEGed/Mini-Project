#ifndef CHATINPUT_H
#define CHATINPUT_H

#include <config.h>
#include <stdbool.h>
#include <stdint.h>

#include "message.h"

// rotate through control, input through button press

struct chatInput {
    char buffer[MAX_INPUT_LENGTH + 1];
    uint8_t length;
    // character under the cursor
    char currentChar;

    void init();
    void tickJoystick(int8_t dirY, bool btnPressed);

    // returns true when a message is ready
    bool hasMessage();
    // fills msg, call after hasMessage()
    void popMessage(Message& out, uint8_t senderID, char name);

   private:
    bool messageReady;
    uint32_t lastMoveMs;
    int8_t lastDirY;
    static const uint32_t REPEAT_DELAY_MS = 200;

    void advance(int8_t dir);
    void commit();
};

#endif