#include "audioReceiver.h"
#include <debug.h>
#include <string.h>

void AudioReceiver::begin() {
    // Initialize the transceiver
    m_xcvr.setup();
    m_xcvr.setMode(RECEIVE);
    
    m_lastSequenceNum = 0;
    m_hasLastSeq = false;
    m_playbackPos = 0;
    m_playbackBufferReady = false;
    m_packetsReceived = 0;
    m_sequenceErrors = 0;
    m_corruptedPackets = 0;
    
    memset(m_playbackBuffer, 128, sizeof(m_playbackBuffer));  // midpoint silence
    
    LOG_INFO("AudioReceiver initialized");
}

void AudioReceiver::update() {
    AudioPacket packet;
    
    // Read available packets from RF24
    if (!m_xcvr.read(&packet, sizeof(AudioPacket))) {
        return;
    }
    
    m_packetsReceived++;
    
    // Check sequence number for drops
    if (m_hasLastSeq) {
        uint8_t expectedSeq = m_lastSequenceNum + 1;
        if (packet.sequenceNum != expectedSeq) {
            // Allow for sequence number wrap-around
            if (!(packet.sequenceNum == 0 && m_lastSequenceNum == 255)) {
                m_sequenceErrors++;
                LOG_ERROR("Sequence error: expected %u, got %u", 
                         expectedSeq, packet.sequenceNum);
            }
        }
    }
    
    m_lastSequenceNum = packet.sequenceNum;
    m_hasLastSeq = true;
    
    // Copy audio data into playback buffer
    const size_t audioDataSize = sizeof(packet.audioData);
    
    for (size_t i = 0; i < audioDataSize; i++) {
        m_playbackBuffer[m_playbackPos] = packet.audioData[i];
        m_playbackPos++;
        
        // Check if playback buffer is full
        if (m_playbackPos >= OUTPUT_BUFFER_SIZE) {
            m_playbackBufferReady = true;
            m_playbackPos = 0;
        }
    }
}

bool AudioReceiver::isPlaybackBufferReady() {
    return m_playbackBufferReady;
}

const uint8_t* AudioReceiver::getPlaybackBuffer() {
    if (!m_playbackBufferReady) {
        return nullptr;
    }
    
    m_playbackBufferReady = false;
    return m_playbackBuffer;
}
