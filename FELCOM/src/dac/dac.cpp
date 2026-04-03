#include "dac.h"

#include <debug.h>
#include <math.h>
#include <string.h>

namespace {
constexpr size_t kSineSamples = 256;
uint8_t g_sineTable[kSineSamples];

uint8_t g_dacPin = 25;
size_t g_sampleIndex = 0;
uint32_t g_lastSampleMicros = 0;
uint32_t g_samplePeriodUs = 0;
bool g_dacReady = false;

// Audio streaming state
bool g_streamingMode = false;
constexpr size_t kMaxAudioSamples = 1024;  // 128ms buffer @ 8kHz
uint8_t g_audioBuffer[kMaxAudioSamples];
size_t g_audioReadPos = 0;
size_t g_audioWritePos = 0;
uint32_t g_streamingSamplePeriodUs = 0;
uint32_t g_lastStreamingSampleUs = 0;
uint8_t g_lastOutputSample = 128;  // Hold last sample for underrun
uint32_t g_underrunCount = 0;      // Track underruns for diagnostics

void buildSineTable() {
    for (size_t i = 0; i < kSineSamples; ++i) {
        const float angle = (2.0f * PI * static_cast<float>(i)) /
                            static_cast<float>(kSineSamples);
        const float normalized = (sinf(angle) + 1.0f) * 127.5f;
        g_sineTable[i] = static_cast<uint8_t>(normalized);
    }
}
}  // namespace

void setupDAC(uint8_t pin, float frequencyHz) {
    if (pin != 25 && pin != 26) {
        LOG_ERROR("Invalid DAC pin %u. Falling back to GPIO 25.", pin);
        pin = 25;
    }

    if (frequencyHz <= 0.0f) {
        LOG_ERROR("Invalid DAC frequency %.2f Hz. Falling back to 440 Hz.",
                  frequencyHz);
        frequencyHz = 440.0f;
    }

    buildSineTable();

    g_dacPin = pin;
    g_sampleIndex = 0;
    g_lastSampleMicros = micros();
    g_samplePeriodUs = static_cast<uint32_t>(
        1000000.0f / (frequencyHz * static_cast<float>(kSineSamples)));
    if (g_samplePeriodUs == 0) {
        g_samplePeriodUs = 1;
    }

    dacWrite(g_dacPin, 128);
    g_dacReady = true;
    g_streamingMode = false;

    LOG_INFO("DAC sine ready: pin=%u, frequency=%.2f Hz", g_dacPin,
             frequencyHz);
}

void updateDACSine() {
    if (!g_dacReady || g_streamingMode) {
        return;
    }

    const uint32_t now = micros();
    while ((now - g_lastSampleMicros) >= g_samplePeriodUs) {
        g_lastSampleMicros += g_samplePeriodUs;
        dacWrite(g_dacPin, g_sineTable[g_sampleIndex]);
        g_sampleIndex = (g_sampleIndex + 1) % kSineSamples;
    }
}

void setupDACStreamingMode(uint8_t pin, uint32_t sampleRateHz) {
    if (pin != 25 && pin != 26) {
        LOG_ERROR("Invalid DAC pin %u. Falling back to GPIO 25.", pin);
        pin = 25;
    }

    if (sampleRateHz == 0) {
        LOG_ERROR("Invalid sample rate. Falling back to 8000 Hz.");
        sampleRateHz = 8000;
    }

    g_dacPin = pin;
    g_streamingSamplePeriodUs = 1000000 / sampleRateHz;
    g_lastStreamingSampleUs = micros();
    g_audioReadPos = 0;
    g_audioWritePos = 0;
    memset(g_audioBuffer, 128, sizeof(g_audioBuffer));  // silence (midpoint)
    
    g_dacReady = true;
    g_streamingMode = true;

    dacWrite(g_dacPin, 128);

    LOG_INFO("DAC streaming mode ready: pin=%u, rate=%lu Hz", pin, sampleRateHz);
}

void playAudioSamples(const uint8_t* samples, size_t count) {
    if (!g_streamingMode || !g_dacReady || samples == nullptr) {
        return;
    }

    // Add samples to circular buffer
    for (size_t i = 0; i < count; i++) {
        g_audioBuffer[g_audioWritePos] = samples[i];
        g_audioWritePos = (g_audioWritePos + 1) % kMaxAudioSamples;
    }
}

void updateDACStreaming() {
    if (!g_streamingMode || !g_dacReady) {
        return;
    }

    // Output samples at the specified rate
    // IMPORTANT: Recapture time each iteration to ensure consistent 125µs intervals
    while ((micros() - g_lastStreamingSampleUs) >= g_streamingSamplePeriodUs) {
        g_lastStreamingSampleUs += g_streamingSamplePeriodUs;

        // Output if there's data available, otherwise hold last sample
        if (g_audioReadPos != g_audioWritePos) {
            g_lastOutputSample = g_audioBuffer[g_audioReadPos];
            dacWrite(g_dacPin, g_lastOutputSample);
            g_audioReadPos = (g_audioReadPos + 1) % kMaxAudioSamples;
        } else {
            // No data available - hold last sample (less jarring than silence)
            dacWrite(g_dacPin, g_lastOutputSample);
            g_underrunCount++;
        }
    }
}

bool isDACStreamingDone() {
    return g_streamingMode && (g_audioReadPos == g_audioWritePos);
}

