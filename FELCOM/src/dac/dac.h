#ifndef DAC_H
#define DAC_H

#include <Arduino.h>
#include <stdint.h>

// Initialize ESP32 DAC output and sine-wave generator.
// Valid DAC pins are GPIO 25 (DAC1) and GPIO 26 (DAC2).
void setupDAC(uint8_t pin = 25, float frequencyHz = 440.0f);

// Call frequently from loop() to keep waveform output running.
void updateDACSine();

// Audio streaming mode - plays samples from buffer instead of sine wave
void setupDACStreamingMode(uint8_t pin = 25, uint32_t sampleRateHz = 8000);

// Supply audio samples for playback (call with buffered audio data)
// samples: pointer to array of 8-bit PCM samples (0-255)
// count: number of samples in array
void playAudioSamples(const uint8_t* samples, size_t count);

// Update DAC streaming playback (call frequently from loop)
void updateDACStreaming();

// Check if all supplied samples have been played
bool isDACStreamingDone();

#endif  // DAC_H
