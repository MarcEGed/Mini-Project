#include <config.h>
#include <debug.h>

#include "display.h"
#include "input.h"
#include "message.h"
#include "chat\chatInput.h"
#include "chat\chatLog.h"
#include "chat\chatRenderer.h"
#include "transceiver.h"

#include "pong/pong.h"

transceiver xcvr;
chatLog     Log;
chatInput   Input;

void setup() {
    loggerSetup();
    //LOG_INFO("Booting node %c (id=0x%02X)", NODE_NAME, SENDER_ID);

    setupDisplay();
    //LOG_INFO("Display ready");

    inputInit();

    xcvr.setup();
    xcvr.setMode(RECEIVE);
    //LOG_INFO("Transceiver ready, listening on channel %d", xcvr.channel);

    Log.init();
    Input.init();

    //LOG_INFO("Boot complete");
}

void loop() {
    int8_t dir     = inputDirectionY();
    bool   btnDown = inputButtonPressed();

    //LOG_INFO("adc: %d", analogRead(JOYSTICK_Y_PIN));
    Input.tickJoystick(dir, btnDown);
    

    //Send if message is ready
    if (Input.hasMessage()) {
        Message msg;
        Input.popMessage(msg, SENDER_ID, NODE_NAME);

        Log.push(msg);  // show own message immediately

        xcvr.setMode(TRANSMIT);
        bool ok = xcvr.write(&msg, sizeof(Message));
        xcvr.setMode(RECEIVE);

        /*if (ok) {
            LOG_INFO("Sent: \"%s\"", msg.text);
        } else {
            LOG_ERROR("Send failed");
        }*/
    }

    //Receive incoming
    Message incoming;
    xcvr.read(&incoming, sizeof(Message));
    if (xcvr.read(&incoming, sizeof(Message))) {
    LOG_INFO("Raw packet - sender: 0x%02X name: %c text: %s", 
             incoming.senderId, 
             incoming.senderName, 
             incoming.text);
    
      if (incoming.senderId != SENDER_ID) {
          Log.push(incoming);
          LOG_INFO("Pushed to log");
      } else {
          LOG_INFO("Ignored own packet");
      }
  }

    // xcvr.read() only fills incoming when radio->available() is true,
    // so check sender_id to confirm a real packet landed
    if (incoming.senderId != 0 && incoming.senderId != SENDER_ID) {
        //LOG_INFO("Received from %c: \"%s\"", incoming.senderName, incoming.text);
        Log.push(incoming);
    }

    // Render
    renderChat(Log, Input);

    delay(30);
}






/*
//PongGame game;
//bool leftPaddleSelected = false;

// transceiver radio;

/*void rotaryEncoderCW()
{
  if (leftPaddleSelected)
  {
    moveLeftPaddle(&game, 1);
  }
  else
  {
    moveRightPaddle(&game, 1);
  }
}

void rotaryEncoderCCW()
{
  if (leftPaddleSelected)
  {
    moveLeftPaddle(&game, -1);
  }
  else
  {
    moveRightPaddle(&game, -1);
  }
}

void rotaryEncoderButtonPressed()
{
  leftPaddleSelected = !leftPaddleSelected;
}
*/
/*void setup(){
  /*#if DEBUG
    loggerSetup();
  #endif
    initializeGame(&game);
    setupDisplay();

  // input choice
  #if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
    setupRotaryEncoder();
  #else
    joystickInit();
  #endif

    // radio.setup();
    // radio.setMode(RECEIVE);
    // pinMode(2, OUTPUT);
  }*/

/*void loop(){
  // delay(500);
  // digitalWrite(2, HIGH);
  // delay(500);
  // digitalWrite(2, LOW);
  //drawGame(&game);
}*/
