#ifndef AUDIO_SENDER_H
#define AUDIO_SENDER_H

#include <transceiver.h>
#include <audioMessage.h>
#include "../adcAcquisition/adcAcquisition.h"
#include <stdint.h>

// Audio sender: acquires samples from microphone and sends them over RF24
class AudioSender {
public:
    // Initialize sender with ADC pin and transmission sample rate
    void begin(uint8_t micPin, uint32_t sampleRateHz = 8000);
    
    // Call frequently from loop to acquire, buffer, and send audio packets
    // Returns true if a packet was sent, false otherwise
    bool update();
    
    // Get statistics
    uint32_t getPacketsSent() const { return m_packetsSent; }
    uint32_t getAcquisitionErrors() const { return m_acquisitionErrors; }
    uint32_t getTransmitErrors() const { return m_transmitErrors; }

private:
    ADCBuffer m_adcBuffer;
    transceiver m_xcvr;
    uint8_t m_sequenceNum;
    uint32_t m_packetsSent;
    uint32_t m_acquisitionErrors;
    uint32_t m_transmitErrors;
    
    // Helper to pack ADC samples into audio packet
    void packAudioPacket(const uint8_t* samples, size_t numSamples, 
                        AudioPacket* outPacket);
};

#endif  // AUDIO_SENDER_H
