#ifndef AUDIO_EXAMPLES_H
#define AUDIO_EXAMPLES_H

/**
 * EXAMPLE USAGE PATTERNS FOR AUDIO STREAMING
 * 
 * This file documents common usage patterns
 * Compile with: copy patterns into actual code, remove comments
 */

// ============================================================================
// EXAMPLE 1: Basic Sender (Microphone → RF24)
// ============================================================================

/*
#include "audioSender.h"
#include <config.h>

AudioSender sender;

void setup() {
    Serial.begin(9600);
    setupDisplay();
    
    // Initialize sender: microphone on GPIO34, 8kHz sample rate
    sender.begin(34, 8000);
    LOG_INFO("Audio sender ready");
}

void loop() {
    // Call continuously - acquires samples and transmits packets
    if (sender.update()) {
        // A packet was successfully sent
        LOG_INFO("Sent packet #%u", sender.getPacketsSent());
    }
    
    // Display stats occasionally
    static uint32_t lastStats = 0;
    if (millis() - lastStats >= 1000) {
        lastStats = millis();
        LOG_INFO("TX: packets=%u, errors=%u", 
                 sender.getPacketsSent(), 
                 sender.getTransmitErrors());
    }
}
*/

// ============================================================================
// EXAMPLE 2: Basic Receiver (RF24 → DAC → Speaker)
// ============================================================================

/*
#include "audioReceiver.h"
#include "dac.h"

AudioReceiver receiver;

void setup() {
    Serial.begin(9600);
    setupDisplay();
    
    receiver.begin();
    
    // Initialize DAC for audio output on GPIO25, 8kHz
    setupDACStreamingMode(25, 8000);
    
    LOG_INFO("Audio receiver ready");
}

void loop() {
    // Receive and buffer incoming audio packets
    receiver.update();
    
    // When playback buffer is ready, send to DAC
    if (receiver.isPlaybackBufferReady()) {
        const uint8_t* audioBuffer = receiver.getPlaybackBuffer();
        playAudioSamples(audioBuffer, AudioReceiver::OUTPUT_BUFFER_SIZE);
    }
    
    // Keep DAC output timing
    updateDACStreaming();
    
    // Monitor link quality
    static uint32_t lastStats = 0;
    if (millis() - lastStats >= 1000) {
        lastStats = millis();
        float lossRate = (receiver.getSequenceErrors() * 100.0f) 
                        / (receiver.getPacketsReceived() + 1);
        LOG_INFO("RX: packets=%u, loss=%.1f%%", 
                 receiver.getPacketsReceived(), lossRate);
    }
}
*/

// ============================================================================
// EXAMPLE 3: Bi-directional Audio (Half-Duplex Time-Multiplexing)
// ============================================================================

/*
#include "audioSender.h"
#include "audioReceiver.h"
#include "dac.h"

AudioSender sender;
AudioReceiver receiver;

void setup() {
    Serial.begin(9600);
    setupDisplay();
    
    sender.begin(34, 8000);
    receiver.begin();
    setupDACStreamingMode(25, 8000);
    
    LOG_INFO("Bi-directional audio ready");
}

void loop() {
    static int cycle = 0;
    
    // Alternate between sending and receiving each loop iteration
    if (cycle % 2 == 0) {
        // Send audio
        sender.update();
    } else {
        // Receive audio
        receiver.update();
        
        if (receiver.isPlaybackBufferReady()) {
            const uint8_t* buffer = receiver.getPlaybackBuffer();
            playAudioSamples(buffer, AudioReceiver::OUTPUT_BUFFER_SIZE);
        }
    }
    
    // Always keep DAC running
    updateDACStreaming();
    
    cycle++;
    
    // Note: This will have gaps in transmission (every other cycle)
    // For true bi-directional, use separate RF24 channels or different devices
}
*/

// ============================================================================
// EXAMPLE 4: With Level Monitoring & AGC
// ============================================================================

/*
#include "audioSender.h"

AudioSender sender;

void setup() {
    sender.begin(34, 8000);
}

uint8_t getMicrophoneLevel() {
    // Simple peak detector on mic input
    static uint8_t maxSample = 0;
    
    // Sample microphone a few times
    for (int i = 0; i < 100; i++) {
        uint16_t adc = analogRead(34);
        uint8_t sample = (adc >> 4) & 0xFF;
        if (sample > maxSample) {
            maxSample = sample;
        }
    }
    
    return maxSample;
}

void loop() {
    sender.update();
    
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck >= 100) {
        lastCheck = millis();
        
        uint8_t level = getMicrophoneLevel();
        double levelDb = 20.0 * log10(level / 255.0);
        LOG_INFO("Mic level: %u (%f dB)", level, levelDb);
        
        if (level < 50) {
            LOG_WARN("Microphone too quiet!");
        }
    }
}
*/

// ============================================================================
// EXAMPLE 5: Graceful Error Recovery
// ============================================================================

/*
#include "audioReceiver.h"

AudioReceiver receiver;
static uint32_t lastPacketTime = 0;
static const uint32_t LINK_TIMEOUT_MS = 1000;

void setup() {
    receiver.begin();
    setupDACStreamingMode(25, 8000);
}

void loop() {
    receiver.update();
    
    if (receiver.getPacketsReceived() > 0) {
        lastPacketTime = millis();
    }
    
    // Check for link timeout
    if ((millis() - lastPacketTime) > LINK_TIMEOUT_MS) {
        LOG_ERROR("Audio link lost!");
        // Could restart RF24, display error, etc.
    }
    
    if (receiver.isPlaybackBufferReady()) {
        const uint8_t* buffer = receiver.getPlaybackBuffer();
        playAudioSamples(buffer, AudioReceiver::OUTPUT_BUFFER_SIZE);
    }
    
    updateDACStreaming();
}
*/

// ============================================================================
// EXAMPLE 6: Compression (4-bit Packing)
// ============================================================================

/*
// Simple 4:1 compression - store 2 samples per byte
uint8_t compress_4bit(uint8_t high, uint8_t low) {
    return ((high >> 4) & 0xF0) | ((low >> 4) & 0x0F);
}

void decompress_4bit(uint8_t compressed, uint8_t& high, uint8_t& low) {
    high = (compressed & 0xF0) << 0;  // High 4 bits
    low = (compressed & 0x0F) << 4;   // Low 4 bits
}

// Usage in sender:
void packCompressed(const uint8_t* samples, AudioPacket* packet) {
    for (int i = 0; i < 14; i++) {
        packet->audioData[i] = compress_4bit(samples[i*2], samples[i*2+1]);
    }
    // Now 28 samples fit in 14 bytes, double the throughput!
}

// Usage in receiver:
void unpackCompressed(const AudioPacket* packet) {
    uint8_t samples[28];
    for (int i = 0; i < 14; i++) {
        decompress_4bit(packet->audioData[i], samples[i*2], samples[i*2+1]);
    }
    playAudioSamples(samples, 28);
}
*/

// ============================================================================
// EXAMPLE 7: Buffer Underrun Prevention
// ============================================================================

/*
#include "audioReceiver.h"

class AudioReceiverWithUnderrunDetection : public AudioReceiver {
public:
    bool hasUnderrun() const { return m_underrunDetected; }
    
    void updateWithDetection() {
        // Track buffer usage
        static uint32_t lastCheck = 0;
        uint32_t now = millis();
        
        if (now - lastCheck >= 100) {
            lastCheck = now;
            
            // If playback buffer empties before new packet arrives
            if (isPlaybackBufferReady() == false && 
                getPacketsReceived() > 0) {
                uint32_t timeSinceLastPacket = now - m_lastPacketMs;
                if (timeSinceLastPacket > 50) {  // More than 50ms since packet
                    m_underrunDetected = true;
                    LOG_ERROR("Potential buffer underrun detected!");
                }
            }
        }
        
        update();
        m_lastPacketMs = millis();
    }
    
private:
    bool m_underrunDetected = false;
    uint32_t m_lastPacketMs = 0;
};

// Usage:
AudioReceiverWithUnderrunDetection receiver;

void loop() {
    receiver.updateWithDetection();
    
    if (receiver.hasUnderrun()) {
        LOG_WARN("Audio is skipping - increase buffer or improve RF link");
    }
    
    // ... rest of loop
}
*/

// ============================================================================
// EXAMPLE 8: Test Mode - Tone Generator on Sender
// ============================================================================

/*
// Generate test tones instead of reading microphone
const uint8_t SINE_TABLE_256[] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170,
    // ... (256-entry sine table from 0-255 range)
};

class TestToneSender {
    size_t m_sineIndex = 0;
    uint8_t m_buffer[128];
    
public:
    void generateTone() {
        for (int i = 0; i < 128; i++) {
            m_buffer[i] = SINE_TABLE_256[m_sineIndex];
            m_sineIndex = (m_sineIndex + 1) % 256;
        }
    }
    
    const uint8_t* getBuffer() { return m_buffer; }
};
*/

// ============================================================================
// EXAMPLE 9: Statistics Display
// ============================================================================

/*
void displayAudioStats(AudioSender* sender, AudioReceiver* receiver) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("AUDIO STATS");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    if (sender) {
        display.setCursor(0, 12);
        display.print("TX:");
        
        char buf[32];
        snprintf(buf, sizeof(buf), "  Pkt: %u, Err: %u",
                 sender->getPacketsSent(),
                 sender->getTransmitErrors());
        display.setCursor(0, 20);
        display.print(buf);
    }
    
    if (receiver) {
        display.setCursor(0, 32);
        display.print("RX:");
        
        char buf[32];
        float loss = (receiver->getSequenceErrors() * 100.0f) / 
                     (receiver->getPacketsReceived() + 1);
        snprintf(buf, sizeof(buf), "  Pkt: %u, Loss: %.1f%%",
                 receiver->getPacketsReceived(),
                 loss);
        display.setCursor(0, 40);
        display.print(buf);
    }
    
    display.display();
}
*/

#endif  // AUDIO_EXAMPLES_H
