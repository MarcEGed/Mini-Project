#include "buttonToneTest.h"
#include "../audio/audioSender.h"
#include "../audio/audioReceiver.h"
#include "../dac/dac.h"
#include "display.h"
#include <debug.h>

// Forward declarations
static AudioSender g_sender;
static AudioReceiver g_receiver;
static uint32_t g_lastStatsUpdateMs = 0;
static uint32_t g_testStartMs = 0;
static uint32_t g_lastButtonCheckMs = 0;
static int g_currentTone = 0;  // 0=idle, 1=tone1, 2=tone2
static uint32_t g_tone1PacketsSent = 0;
static uint32_t g_tone2PacketsSent = 0;

#define BUTTON_DEBOUNCE_MS 50
#define DISPLAY_UPDATE_INTERVAL_MS 500

static void renderButtonToneStats() {
#if BUTTON_TONE_TEST_TX
    // Transmitter stats
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("TONE TEST [TX]");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    char line[22];
    
    // Show current tone status
    if (g_currentTone == 0) {
        snprintf(line, sizeof(line), "Status: IDLE");
    } else if (g_currentTone == 1) {
        snprintf(line, sizeof(line), "Status: 440Hz");
    } else {
        snprintf(line, sizeof(line), "Status: 880Hz");
    }
    display.setCursor(0, 16);
    display.print(line);
    
    // Total packets
    snprintf(line, sizeof(line), "Total: %lu",
             (unsigned long)(g_tone1PacketsSent + g_tone2PacketsSent));
    display.setCursor(0, 28);
    display.print(line);
    
    // Button hints
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print("P12:440Hz P13:880Hz");
    
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
    display.print("TONE TEST [RX]");
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

static void updateButtonStates() {
    uint32_t now = millis();
    if ((now - g_lastButtonCheckMs) < BUTTON_DEBOUNCE_MS) {
        return;
    }
    g_lastButtonCheckMs = now;
    
    int button1State = digitalRead(BUTTON1_PIN);
    int button2State = digitalRead(BUTTON2_PIN);
    
    if (button1State == LOW) {
        // Button 1 pressed
        if (g_currentTone != 1) {
            g_currentTone = 1;
            g_sender.setSineWaveMode(TONE1_FREQUENCY);
            LOG_INFO("Button 1: Switched to 440Hz tone");
        }
    } else if (button2State == LOW) {
        // Button 2 pressed
        if (g_currentTone != 2) {
            g_currentTone = 2;
            g_sender.setSineWaveMode(TONE2_FREQUENCY);
            LOG_INFO("Button 2: Switched to 880Hz tone");
        }
    } else {
        // No button pressed
        if (g_currentTone != 0) {
            g_currentTone = 0;
            g_sender.stop();  // Stop transmission (don't send anything)
            LOG_INFO("Button released: Idle mode");
        }
    }
}

void buttonToneTestSetup() {
    g_testStartMs = millis();
    g_lastStatsUpdateMs = millis();
    g_lastButtonCheckMs = millis();
    g_currentTone = 0;
    g_tone1PacketsSent = 0;
    g_tone2PacketsSent = 0;
    
    // Set up button pins as inputs with pull-up
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    
#if BUTTON_TONE_TEST_TX
    LOG_INFO("Starting button tone test - TX mode");
    g_sender.begin(34, 8000);  // Use ADC pin but won't use it in generated mode
    LOG_INFO("User can press button 12 (440Hz) or 13 (880Hz)");
#else
    LOG_INFO("Starting button tone test - RX mode (will play whatever it receives)");
    g_receiver.begin();
    setupDACStreamingMode(25, 8000);  // GPIO 25, 8kHz
#endif
}

void buttonToneTestLoop() {
#if BUTTON_TONE_TEST_TX
    // Update button states on sender
    updateButtonStates();
    
    // Send audio packets
    g_sender.update();
    
    // Count packets by type
    static uint32_t lastPacketCount = 0;
    uint32_t currentPacketCount = g_sender.getPacketsSent();
    if (currentPacketCount != lastPacketCount) {
        if (g_currentTone == 1) {
            g_tone1PacketsSent++;
        } else if (g_currentTone == 2) {
            g_tone2PacketsSent++;
        }
        lastPacketCount = currentPacketCount;
    }
    
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
    
    // Update display periodically
    uint32_t now = millis();
    if ((now - g_lastStatsUpdateMs) >= DISPLAY_UPDATE_INTERVAL_MS) {
        renderButtonToneStats();
        g_lastStatsUpdateMs = now;
    }
}
