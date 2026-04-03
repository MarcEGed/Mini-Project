#include "adcAcquisition.h"
#include <debug.h>

void ADCBuffer::begin(uint8_t pin, uint32_t sampleRateHz) {
    if (sampleRateHz == 0) {
        LOG_ERROR("Invalid sample rate");
        return;
    }
    
    m_adcPin = pin;
    m_samplePeriodUs = 1000000 / sampleRateHz;
    m_lastSampleTimeUs = micros();
    m_writeBufferIdx = 0;
    m_writePos = 0;
    m_readBufferReady = false;
    m_running = true;
    m_gainMultiplier = 2.0f;  // Default +6dB gain for microphone
    
    // Initialize buffers
    memset(m_buffers, 0, sizeof(m_buffers));
    
    LOG_INFO("ADC initialized: pin=%u, rate=%lu Hz, period=%lu us, gain=%.1fx", 
             pin, sampleRateHz, m_samplePeriodUs, m_gainMultiplier);
}

void ADCBuffer::update() {
    if (!m_running) return;
    
    uint32_t now = micros();
    
    // Sample when enough time has passed
    // IMPORTANT: Recapture time each iteration to avoid jitter from stale 'now'
    while ((micros() - m_lastSampleTimeUs) >= m_samplePeriodUs) {
        m_lastSampleTimeUs += m_samplePeriodUs;
        
        // Read ADC value (12-bit ADC on ESP32, range 0-4095)
        // The MAX4466 operates around 1.65V (midpoint), which is ~2048 in 12-bit
        uint16_t adcValue = analogRead(m_adcPin);
        
        // Convert from centered 12-bit to signed, then scale to 8-bit with gain
        // Subtracting 2048 centers around 0, then we have range ~-2048 to +2047
        int16_t centered = (int16_t)adcValue - 2048;
        
        // Scale to 8-bit range and apply gain
        // Scale: divide by 16 to get ~-128 to +127, then multiply by gain
        int16_t scaled = (int16_t)((centered / 16.0f) * m_gainMultiplier);
        
        // Clamp to valid 0-255 range (offset by 128 for 8-bit DAC)
        int16_t clamped = scaled + 128;
        if (clamped < 0) clamped = 0;
        if (clamped > 255) clamped = 255;
        
        uint8_t sample = (uint8_t)clamped;
        
        // Store in current write buffer
        m_buffers[m_writeBufferIdx][m_writePos] = sample;
        m_writePos++;
        
        // Check if buffer is full
        if (m_writePos >= BUFFER_SIZE) {
            // Mark this buffer as ready for reading
            m_readBufferReady = true;
            
            // Switch to other buffer for writing
            m_writeBufferIdx = (m_writeBufferIdx + 1) % NUM_BUFFERS;
            m_writePos = 0;
        }
    }
}

bool ADCBuffer::isBufferReady() {
    return m_readBufferReady;
}

const uint8_t* ADCBuffer::getReadBuffer() {
    if (!m_readBufferReady) return nullptr;
    
    // Return the buffer that was just filled (not the one we're currently writing to)
    size_t readBufferIdx = (m_writeBufferIdx + 1) % NUM_BUFFERS;
    m_readBufferReady = false;
    return m_buffers[readBufferIdx];
}

void ADCBuffer::reset() {
    m_running = false;
    m_readBufferReady = false;
    m_writePos = 0;
    m_writeBufferIdx = 0;
    memset(m_buffers, 0, sizeof(m_buffers));
}
