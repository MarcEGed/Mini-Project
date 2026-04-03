#include "audioTest.h"
#include "../dac/dac.h"
#include "display.h"

#include "../audio/audioSender.h"
#include "../audio/audioReceiver.h"

// Forward declarations
static AudioSender g_sender;
static AudioReceiver g_receiver;
static uint32_t g_lastStatsUpdateMs = 0;
static uint32_t g_testStartMs = 0;

static void renderAudioStats() {
#if AUDIO_TEST_TX
    // Transmitter stats
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("AUDIO TEST [TX]");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    char line[22];
    snprintf(line, sizeof(line), "Packets: %lu",
             (unsigned long)g_sender.getPacketsSent());
    display.setCursor(0, 16);
    display.print(line);
    
    snprintf(line, sizeof(line), "Acq Err: %lu",
             (unsigned long)g_sender.getAcquisitionErrors());
    display.setCursor(0, 28);
    display.print(line);
    
    snprintf(line, sizeof(line), "TX Err: %lu",
             (unsigned long)g_sender.getTransmitErrors());
    display.setCursor(0, 40);
    display.print(line);
    
    uint32_t elapsedMs = millis() - g_testStartMs;
    snprintf(line, sizeof(line), "Time: %lu ms", (unsigned long)elapsedMs);
    display.setCursor(0, 52);
    display.print(line);
    
#else
    // Receiver stats
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("AUDIO TEST [RX]");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    char line[22];
    snprintf(line, sizeof(line), "Packets: %lu",
             (unsigned long)g_receiver.getPacketsReceived());
    display.setCursor(0, 16);
    display.print(line);
    
    snprintf(line, sizeof(line), "Seq Err: %lu",
             (unsigned long)g_receiver.getSequenceErrors());
    display.setCursor(0, 28);
    display.print(line);
    
    snprintf(line, sizeof(line), "Bad: %lu",
             (unsigned long)g_receiver.getCorruptedPackets());
    display.setCursor(0, 40);
    display.print(line);
    
    uint32_t elapsedMs = millis() - g_testStartMs;
    snprintf(line, sizeof(line), "Time: %lu ms", (unsigned long)elapsedMs);
    display.setCursor(0, 52);
    display.print(line);
#endif
    
    display.display();
}

void audioTestSetup() {
    g_testStartMs = millis();
    g_lastStatsUpdateMs = millis();

#if AUDIO_TEST_TX
    LOG_INFO("Starting audio test - TX mode (microphone to RF24)");
    g_sender.begin(AUDIO_MIC_PIN, AUDIO_SAMPLE_RATE);
    g_sender.setAudioMode();  // Explicitly enable ADC mode
    
    // Set microphone gain for better signal capture
    // Typical MAX4466: 25-65dB adjustable, default ~40dB
    // If audio is still buzzing, try 1.0f (0dB), 2.0f (+6dB), 4.0f (+12dB)
    g_sender.setMicrophoneGain(2.0f);  // +6dB gain
    
    LOG_INFO("Microphone gain set to +6dB");
#else
    LOG_INFO("Starting audio test - RX mode (RF24 to speaker)");
    g_receiver.begin();
    setupDACStreamingMode(AUDIO_DAC_PIN, AUDIO_SAMPLE_RATE);
#endif
}

void audioTestLoop() {
#if AUDIO_TEST_TX
    // Transmitter side: continuously acquire and send audio
    g_sender.update();
    
#else
    // Receiver side: receive audio and play through DAC
    g_receiver.update();
    
    // Get audio buffer if ready and send to DAC
    if (g_receiver.isPlaybackBufferReady()) {
        const uint8_t* buffer = g_receiver.getPlaybackBuffer();
        if (buffer != nullptr) {
            playAudioSamples(buffer, AudioReceiver::OUTPUT_BUFFER_SIZE);
        }
    }
    
    // Update DAC output
    updateDACStreaming();
#endif
    
    // Update display stats periodically
    uint32_t now = millis();
    if ((now - g_lastStatsUpdateMs) >= 500) {
        g_lastStatsUpdateMs = now;
        renderAudioStats();
    }
}
