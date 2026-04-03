#ifndef AUDIO_TEST_H
#define AUDIO_TEST_H

#include "config.h"
#include "debug.h"
#include <Arduino.h>

// Audio streaming test mode - sender streams microphone, receiver plays speaker
// Configure AUDIO_TEST_TX to switch between transmitter and receiver

// ADC pin for microphone input (sender only)
#define AUDIO_MIC_PIN 34

// DAC pin for speaker output (receiver only)
#define AUDIO_DAC_PIN 25

// Sample rate for audio streaming (Hz)
#define AUDIO_SAMPLE_RATE 8000

// Enable transmit mode (1) or receive mode (0)
// This should be different on sender and receiver devices
#define AUDIO_TEST_TX 0

void audioTestSetup();
void audioTestLoop();

#endif  // AUDIO_TEST_H
