#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

struct transceiver;

// ===========================================================================
// Non-blocking real-time audio for FELCOM (no FreeRTOS tasks).
//
// Audio rides the existing transceiver: captured mic samples are packed into
// AudioPayloads and sent through xcvr.audioTx() (CRC + XOR block FEC), and
// received frames come back through xcvr.audioRx(). Capture, transmit, receive
// and playback are cooperative steps pumped from the main loop via update();
// none of them block (I2S read uses timeout 0, playback is micros-paced), so
// FHSS channel hopping in the main loop still happens on time.
//
// Half-duplex: a node is either TALKING (mic -> radio) or LISTENING
// (radio -> speaker), toggled from the AUDIO screen. Mic capture is stereo and
// uses slot 0 — ONLY_LEFT returns silence on this board (ESP32 I2S quirk).
// ===========================================================================
namespace audio {

// One-time init at boot: I2S mic (stereo) and the DAC pin.
void begin(transceiver* xcvr);

// Enter/leave the AUDIO screen. enter() starts in LISTENING; leave() silences
// the DAC and returns the radio to RECEIVE.
void enter();
void leave();

// Switch role. true = TALK (capture + transmit), false = LISTEN (receive + play).
void setTalking(bool talking);
bool isTalking();

// Pump one cooperative step. Call every loop() while the AUDIO screen is open.
void update();

// Diagnostics for the opt-in serial debug line.
uint32_t txPayloads();
uint32_t rxPayloads();

// Mic bring-up diagnostic (enabled via MIC_TEST_MODE in config.h). Blocks
// forever: reads the I2S mic, prints each slot's signal span to serial (~2x/sec)
// and plays slot 0 to the DAC so you can both see and hear whether the mic
// works. No radio, sync or FEC involved — use it to isolate microphone issues.
// Never returns.
void micProbe();

}  // namespace audio

#endif  // AUDIO_H
