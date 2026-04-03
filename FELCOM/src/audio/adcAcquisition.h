#ifndef ADC_ACQUISITION_H
#define ADC_ACQUISITION_H

#include <Arduino.h>
#include <stdint.h>

// ADC acquisition module for microphone input with circular buffer and DMA
// Uses dual buffers to allow continuous acquisition while reading

class ADCBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 128;    // 128 samples per buffer
    static constexpr size_t NUM_BUFFERS = 2;      // dual buffer for continuous acquisition
    
    // Initialize ADC on specified pin with sample rate
    // pin: ADC pin (e.g., 34, 35, 36 on ESP32)
    // sampleRateHz: desired sample rate (e.g., 8000, 16000 Hz)
    void begin(uint8_t pin, uint32_t sampleRateHz = 8000);
    
    // Call frequently to process new samples into buffers
    void update();
    
    // Check if a complete buffer is ready to read
    bool isBufferReady();
    
    // Get pointer to ready buffer (call after isBufferReady() returns true)
    // Caller must copy data before calling update() again
    const uint8_t* getReadBuffer();
    
    // Reset buffer state
    void reset();
    
    // Set microphone gain (0.5 = -6dB, 1.0 = 0dB, 2.0 = +6dB, 4.0 = +12dB)
    void setGain(float gainMultiplier) { m_gainMultiplier = gainMultiplier; }
    
    // Get current acquisition state
    bool isRunning() const { return m_running; }

private:
    uint8_t m_adcPin;
    uint32_t m_samplePeriodUs;
    uint32_t m_lastSampleTimeUs;
    float m_gainMultiplier = 1.0f;
    
    uint8_t m_buffers[NUM_BUFFERS][BUFFER_SIZE];
    size_t m_writeBufferIdx;     // which buffer we're currently writing to
    size_t m_writePos;           // position within current write buffer
    bool m_readBufferReady;      // is the read buffer complete?
    bool m_running;
};

#endif  // ADC_ACQUISITION_H
