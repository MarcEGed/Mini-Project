#include "audioSender.h"
#include <debug.h>
#include <config.h>
#include <string.h>

void AudioSender::begin(uint8_t micPin, uint32_t sampleRateHz) {
    // Initialize the transceiver first
    m_xcvr.setup();
    m_xcvr.setMode(TRANSMIT);
    
    // Initialize ADC buffer
    m_adcBuffer.begin(micPin, sampleRateHz);
    
    m_sequenceNum = 0;
    m_packetsSent = 0;
    m_acquisitionErrors = 0;
    m_transmitErrors = 0;
    
    LOG_INFO("AudioSender initialized: mic_pin=%u, sample_rate=%lu Hz", 
             micPin, sampleRateHz);
}

bool AudioSender::update() {
    // Update ADC buffer acquisition
    m_adcBuffer.update();
    
    // Check if a complete buffer is ready
    if (!m_adcBuffer.isBufferReady()) {
        return false;
    }
    
    // Get the filled buffer
    const uint8_t* audioSamples = m_adcBuffer.getReadBuffer();
    if (audioSamples == nullptr) {
        m_acquisitionErrors++;
        return false;
    }
    
    // Create and send audio packet
    AudioPacket packet;
    packAudioPacket(audioSamples, ADCBuffer::BUFFER_SIZE, &packet);
    
    // Send packet
    bool success = m_xcvr.write(&packet, sizeof(AudioPacket));
    
    if (!success) {
        m_transmitErrors++;
        LOG_ERROR("Failed to transmit audio packet %u", m_sequenceNum);
        return false;
    }
    
    m_sequenceNum++;
    m_packetsSent++;
    
    return true;
}

void AudioSender::packAudioPacket(const uint8_t* samples, size_t numSamples,
                                  AudioPacket* outPacket) {
    outPacket->senderId = SENDER_ID;
    outPacket->sequenceNum = m_sequenceNum;
    outPacket->reserved = 0;
    
    // Copy up to 28 bytes of samples (or whatever fits in audioData)
    size_t copySize = (numSamples > sizeof(outPacket->audioData)) 
                    ? sizeof(outPacket->audioData) 
                    : numSamples;
    
    memcpy(outPacket->audioData, samples, copySize);
    
    // Zero pad remaining space if needed
    if (copySize < sizeof(outPacket->audioData)) {
        memset(outPacket->audioData + copySize, 0, 
               sizeof(outPacket->audioData) - copySize);
    }
}
