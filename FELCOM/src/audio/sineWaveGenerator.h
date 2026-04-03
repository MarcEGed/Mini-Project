#ifndef SINE_WAVE_GENERATOR_H
#define SINE_WAVE_GENERATOR_H

#include <stdint.h>
#include <math.h>

// Generate sine wave samples at specified frequency and sample rate
class SineWaveGenerator {
public:
    SineWaveGenerator() 
        : m_frequency(0), m_sampleRate(0), m_phase(0.0f), m_amplitude(127.0f) {}
    
    // Initialize with frequency (Hz) and sample rate (Hz)
    void init(uint32_t frequencyHz, uint32_t sampleRateHz) {
        m_frequency = frequencyHz;
        m_sampleRate = sampleRateHz;
        m_phase = 0.0f;
        // Phase increment per sample: (2π * frequency) / sampleRate
        m_phaseIncrement = (2.0f * M_PI * frequencyHz) / sampleRateHz;
    }
    
    // Generate next sample (0-255, centered at 128)
    uint8_t nextSample() {
        if (m_frequency == 0) return 128;
        
        // Generate sine wave: 128 + 127*sin(phase)
        float sine = sinf(m_phase);
        uint8_t sample = (uint8_t)(128.0f + (m_amplitude * sine));
        
        // Advance phase
        m_phase += m_phaseIncrement;
        
        // Wrap phase to avoid floating point overflow
        if (m_phase > 2.0f * M_PI) {
            m_phase -= 2.0f * M_PI;
        }
        
        return sample;
    }
    
    // Generate multiple samples into buffer
    void generateSamples(uint8_t* buffer, size_t count) {
        for (size_t i = 0; i < count; i++) {
            buffer[i] = nextSample();
        }
    }
    
    // Set amplitude (0-127, default 127 for full range)
    void setAmplitude(float amp) {
        if (amp > 127.0f) amp = 127.0f;
        if (amp < 0.0f) amp = 0.0f;
        m_amplitude = amp;
    }
    
    // Reset phase to 0
    void reset() {
        m_phase = 0.0f;
    }
    
    uint32_t getFrequency() const { return m_frequency; }
    
private:
    uint32_t m_frequency;
    uint32_t m_sampleRate;
    float m_phase;
    float m_phaseIncrement;
    float m_amplitude;
};

#endif  // SINE_WAVE_GENERATOR_H
