#include <config.h>
#include <debug.h>

#include "encoders/encoders.h"
#include "display/display.h"
#include "pong/pong.h"

PongGame game;
bool leftPaddleSelected = false;

void rotaryEncoderCW()
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

void setup()
{
#if DEBUG
  loggerSetup();
#endif
  initializeGame(&game);
  setupDisplay();
  setupRotaryEncoder();
}

void loop()
{
  drawGame(&game);
}
