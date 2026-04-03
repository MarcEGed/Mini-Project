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
    
    // Initialize buffers
    memset(m_buffers, 0, sizeof(m_buffers));
    
    LOG_INFO("ADC initialized: pin=%u, rate=%lu Hz, period=%lu us", 
             pin, sampleRateHz, m_samplePeriodUs);
}

void ADCBuffer::update() {
    if (!m_running) return;
    
    uint32_t now = micros();
    
    // Sample when enough time has passed
    while ((now - m_lastSampleTimeUs) >= m_samplePeriodUs) {
        m_lastSampleTimeUs += m_samplePeriodUs;
        
        // Read ADC value (12-bit ADC on ESP32, range 0-4095)
        // Convert to 8-bit (0-255)
        uint16_t adcValue = analogRead(m_adcPin);
        uint8_t sample = (adcValue >> 4) & 0xFF;  // Convert 12-bit to 8-bit
        
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
