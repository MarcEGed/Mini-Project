#ifndef DAC_H
#define DAC_H

#include <Arduino.h>

// Initialize ESP32 DAC output and sine-wave generator.
// Valid DAC pins are GPIO 25 (DAC1) and GPIO 26 (DAC2).
void setupDAC(uint8_t pin = 25, float frequencyHz = 440.0f);

// Call frequently from loop() to keep waveform output running.
void updateDACSine();

#endif  // DAC_H
