#ifndef BUTTON_TONE_TEST_H
#define BUTTON_TONE_TEST_H

#include <stdint.h>

// Set to 1 for sender (TX), 0 for receiver (RX)
#define BUTTON_TONE_TEST_TX 0

// Button pins for tone generation
#define BUTTON1_PIN 12   // Button 1: Generate 440 Hz (A note)
#define BUTTON2_PIN 13   // Button 2: Generate 880 Hz (A note one octave higher)

// Frequencies (Hz)
#define TONE1_FREQUENCY 440   // A4 (musical note)
#define TONE2_FREQUENCY 880   // A5 (musical note)

// Initialize button tone test
void buttonToneTestSetup();

// Call continuously from loop
void buttonToneTestLoop();

#endif  // BUTTON_TONE_TEST_H
