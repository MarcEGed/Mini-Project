#ifndef AUDIO_UI_H
#define AUDIO_UI_H

#include <stdint.h>

// Pump the audio subsystem and handle input for the AUDIO screen.
// SELECT toggles TALK/LISTEN. Returns true only when the role changed (redraw
// then) — never redraw every loop, the OLED flush would stall playback.
// Call every loop() while the AUDIO screen is open.
bool audioUITickInput(int8_t dirY, bool btnDown);

void audioUIInitDisplay();
void audioUIUpdate();

#endif  // AUDIO_UI_H
