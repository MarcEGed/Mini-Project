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
// (radio -> speaker), toggled from the AUDIO screen. Mic capture is stereo;
// AUDIO_I2S_SLOT_INDEX selects which slot to use because ONLY_LEFT returns
// silence on this board (ESP32 I2S quirk).
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
struct Stats {
    uint32_t txPayloads;
    uint32_t rxPayloads;
    uint32_t i2sReads;
    uint32_t i2sEmptyReads;
    uint32_t micSamples;
    uint32_t micClippedLow;
    uint32_t micClippedHigh;
    uint32_t rxUnderruns;
    uint32_t rxOverruns;
    uint16_t rxLevel;
    int32_t micPeak;
};

uint32_t txPayloads();
uint32_t rxPayloads();
Stats stats();

}  // namespace audio

#endif  // AUDIO_H
