#include <config.h>
#include <debug.h>
#include <encoders.h>
#include <display.h>
#include <transceiver.h>
#include <joystick.h>

#include "pong/pong.h"

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
void setup(){
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
    // pinMode(2, OUTPUT);*/
  }

void loop(){
  // delay(500);
  // digitalWrite(2, HIGH);
  // delay(500);
  // digitalWrite(2, LOW);
  //drawGame(&game);
}


