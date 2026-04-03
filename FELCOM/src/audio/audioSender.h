#ifndef AUDIO_SENDER_H
#define AUDIO_SENDER_H

#include <transceiver.h>
#include <audioMessage.h>
#include "adcAcquisition.h"
#include "sineWaveGenerator.h"
#include <stdint.h>

// Audio sender: acquires samples from microphone and sends them over RF24
class AudioSender {
public:
    // Initialize sender with ADC pin and transmission sample rate
    void begin(uint8_t micPin, uint32_t sampleRateHz = 8000);
    
    // Call frequently from loop to acquire, buffer, and send audio packets
    // Returns true if a packet was sent, false otherwise
    bool update();
    
    // Adjust microphone gain (0.5 = -6dB, 1.0 = 0dB, 2.0 = +6dB, 4.0 = +12dB)
    void setMicrophoneGain(float gainMultiplier) { m_adcBuffer.setGain(gainMultiplier); }
    
    // Switch to generated sine wave mode
    void setSineWaveMode(uint32_t frequencyHz) {
        m_useGeneratedAudio = true;
        m_isStopped = false;
        m_sineGenerator.init(frequencyHz, m_sampleRate);
    }
    
    // Switch back to microphone (ADC) mode
    void setAudioMode() {
        m_useGeneratedAudio = false;
        m_isStopped = false;
    }
    
    // Stop all transmission (don't send anything)
    void stop() {
        m_isStopped = true;
    }
    
    // Get statistics
    uint32_t getPacketsSent() const { return m_packetsSent; }
    uint32_t getAcquisitionErrors() const { return m_acquisitionErrors; }
    uint32_t getTransmitErrors() const { return m_transmitErrors; }

private:
    ADCBuffer m_adcBuffer;
    SineWaveGenerator m_sineGenerator;
    transceiver m_xcvr;
    uint8_t m_sequenceNum;
    uint32_t m_packetsSent;
    uint32_t m_acquisitionErrors;
    uint32_t m_transmitErrors;
    uint32_t m_sampleRate;
    bool m_useGeneratedAudio;
    bool m_isStopped;
    
    // Helper to pack ADC samples into audio packet
    void packAudioPacket(const uint8_t* samples, size_t numSamples, 
                        AudioPacket* outPacket);
};

#endif  // AUDIO_SENDER_H
