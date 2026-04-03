#ifndef AUDIO_MESSAGE_H
#define AUDIO_MESSAGE_H

#include <config.h>
#include <stdint.h>

// Audio packet format for streaming compressed/raw audio over RF24
// Payload: 20 bytes of audio samples (10x 16-bit samples or 20x 8-bit samples)
// Total packet size: 32 bytes (matches RF24 max payload)

struct __attribute__((packed)) AudioPacket {
    uint8_t senderId;             // sender identifier
    uint8_t sequenceNum;          // sequence number for packet ordering (0-255)
    uint16_t reserved;            // reserved for future use, alignment
    uint8_t audioData[28];        // 28 bytes of audio samples
                                  // Can be: 14x 16-bit PCM samples or 28x 8-bit PCM samples
};

static_assert(sizeof(AudioPacket) == 32, "AudioPacket must be exactly 32 bytes for RF24");

#endif  // AUDIO_MESSAGE_H
