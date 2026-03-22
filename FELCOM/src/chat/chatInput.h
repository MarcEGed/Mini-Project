#ifndef CHATINPUT_H
#define CHATINPUT_H

#include "message.h"
#include <stdint.h>
#include <stdbool.h>

//rotate through control, input through button press
#define MAX_INPUT_LENGTH 25 //1 byte space for termination character, might be needed

struct chatInput{
    char   buffer[MAX_INPUT_LENGTH + 1];
    uint8_t length;
    char   currentChar; //character under the cursor

    void init();
    void tickJoystick(int8_t dirY, bool btnPressed);

    bool hasMessage(); //returns true when a message is ready
    void popMessage(Message& out, uint8_t senderID, const char* name); //fills msg, call after hasMessage()

private:
    bool    messageReady;
    uint32_t lastMoveMs;
    static const uint32_t REPEAT_DELAY_MS = 200;

    void advance(int8_t dir);
    void commit();
};


#endif