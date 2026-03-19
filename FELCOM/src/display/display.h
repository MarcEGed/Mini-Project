#ifndef DISPLAY_H
#define DISPLAY_H

#include <config.h>
#include <debug.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void setupDisplay();

#endif // DISPLAY_H