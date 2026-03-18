#include <config.h>
#include <debug.h>

#include "display/display.h"
#include "pong/pong.h"

void setup()
{
#if DEBUG
  loggerSetup();
#endif
  setupDisplay();
}

GamePacket game = {64, 32, 25, 25};

void loop()
{
  drawGame(&game);
}
