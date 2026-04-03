#ifndef AUDIO_RECEIVER_H
#define AUDIO_RECEIVER_H

#include <transceiver.h>
#include <audioMessage.h>
#include <stdint.h>

// Audio receiver: receives audio packets from RF24 and buffers them for playback
class AudioReceiver {
public:
    static constexpr size_t OUTPUT_BUFFER_SIZE = 128;  // samples to accumulate before playback
    
    // Initialize receiver
    void begin();
    
    // Call frequently from loop to receive and buffer audio packets
    void update();
    
    // Check if audio samples are ready for playback
    bool isPlaybackBufferReady();
    
    // Get pointer to playback buffer (copy data immediately)
    // Buffer contains OUTPUT_BUFFER_SIZE samples
    const uint8_t* getPlaybackBuffer();
    
    // Get statistics
    uint32_t getPacketsReceived() const { return m_packetsReceived; }
    uint32_t getSequenceErrors() const { return m_sequenceErrors; }
    uint32_t getCorruptedPackets() const { return m_corruptedPackets; }

private:
    transceiver m_xcvr;
    uint8_t m_lastSequenceNum;
    bool m_hasLastSeq;
    
    uint8_t m_playbackBuffer[OUTPUT_BUFFER_SIZE];
    size_t m_playbackPos;
    bool m_playbackBufferReady;
    
    uint32_t m_packetsReceived;
    uint32_t m_sequenceErrors;
    uint32_t m_corruptedPackets;
};

#endif  // AUDIO_RECEIVER_H
