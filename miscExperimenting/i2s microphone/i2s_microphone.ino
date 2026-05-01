/*
 * INMP441 I2S Mic → ESP32 DAC → LM386 → Speaker
 * * INMP441 Wiring:
 * VDD  --> ESP32 3.3V
 * GND  --> ESP32 GND
 * L/R  --> ESP32 GND    (MUST be grounded)
 * SCK  --> ESP32 GPIO14
 * WS   --> ESP32 GPIO15
 * SD   --> ESP32 GPIO32
 *
 * LM386:
 * IN   --> *** PUT A 10K RESISTOR BETWEEN GPIO25 AND THIS PIN ***
 * VCC  --> ESP32 5V
 * GND  --> ESP32 GND
 * OUT  --> Speaker +
 * Speaker - --> GND
 */

#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/dac.h"

// ── Pins ──────────────────────────────────────────────────────────────────────
#define I2S_SCK   14
#define I2S_WS    15
#define I2S_SD    32

// ── Settings ──────────────────────────────────────────────────────────────────
#define SAMPLE_RATE     16000
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     512
#define GAIN            32       // Lowered from 32. Increase slowly if it's too quiet.
// ─────────────────────────────────────────────────────────────────────────────

static int32_t i2sBuf[DMA_BUF_LEN];

// Ring buffer
#define RING_SIZE  8192
#define RING_MASK  (RING_SIZE - 1)
static volatile uint8_t  ring[RING_SIZE];
static volatile uint32_t ringHead = 0;
static volatile uint32_t ringTail = 0;

// ── I2S mic setup ─────────────────────────────────────────────────────────────
void setupMicI2S() {
    Serial.println("  Installing I2S driver...");

    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate          = SAMPLE_RATE;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = DMA_BUF_COUNT;
    cfg.dma_buf_len          = DMA_BUF_LEN;
    cfg.use_apll             = false;   
    cfg.tx_desc_auto_clear   = false;
    cfg.fixed_mclk           = 0;

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("  ERROR: i2s_driver_install failed: %d\n", err);
        return;
    }
    Serial.println("  I2S driver installed OK");

    i2s_pin_config_t pins = {};
    pins.bck_io_num   = I2S_SCK;
    pins.ws_io_num    = I2S_WS;
    pins.data_in_num  = I2S_SD;
    pins.data_out_num = I2S_PIN_NO_CHANGE;

    err = i2s_set_pin(I2S_NUM_0, &pins);
    if (err != ESP_OK) {
        Serial.printf("  ERROR: i2s_set_pin failed: %d\n", err);
        return;
    }
    Serial.println("  I2S pins set OK");

    i2s_zero_dma_buffer(I2S_NUM_0);
    Serial.println("  DMA buffer zeroed OK");
    Serial.println("INMP441 ready!");
}

// Add this global variable above your micTask
static int32_t dc_offset = 0;

// ── Mic task ──────────────────────────────────────────────────────────────────
void micTask(void* param) {
    Serial.println("Mic task started on Core 0");

    while (true) {
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, i2sBuf, sizeof(i2sBuf), &bytesRead, portMAX_DELAY);
        uint32_t count = bytesRead / 4;

        for (uint32_t i = 0; i < count; i++) {
              // 1. Extract 24-bit sample
              int32_t sample24 = i2sBuf[i] >> 8;

              // 2. Calculate the moving average (DC offset)
              // We use a simple Exponential Moving Average (EMA)
              dc_offset = (dc_offset * 63 + sample24) / 64; 

              // 3. Remove the DC offset from the current sample
              int32_t centered_sample = sample24 - dc_offset;

              // 4. Apply gain and scale down to 8-bit
              int32_t scaled = (centered_sample * GAIN) >> 16;

              // 5. Center perfectly in the DAC's 0-255 range
              int32_t val = 128 + scaled;

              // Strict clamping
              if (val > 255) val = 255;
              if (val <   0) val = 0;

              uint32_t nextHead = (ringHead + 1) & RING_MASK;
              if (nextHead != ringTail) {
                  ring[ringHead] = (uint8_t)val;
                  ringHead = nextHead;
              }
          }
        }
    }


// ── DAC task ──────────────────────────────────────────────────────────────────
void dacTask(void* param) {
    dac_output_enable(DAC_CHANNEL_1); // GPIO25
    dac_output_voltage(DAC_CHANNEL_1, 128); // Initialize to center voltage
    Serial.println("DAC task started on Core 1");

    const uint32_t periodUs     = 1000000UL / SAMPLE_RATE;
    const uint32_t samplesPerMs = SAMPLE_RATE / 1000;
    uint32_t lastUs = micros();

    while (true) {
        for (uint32_t s = 0; s < samplesPerMs; s++) {
            uint32_t now;
            do { now = micros(); } while ((now - lastUs) < periodUs);
            lastUs = now;

            if (ringTail != ringHead) {
                dac_output_voltage(DAC_CHANNEL_1, ring[ringTail]);
                ringTail = (ringTail + 1) & RING_MASK;
            }
        }
        taskYIELD();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== INMP441 Passthrough ===");
    Serial.println("Step 1: Setting up mic I2S...");

    setupMicI2S();

    Serial.println("Step 2: Starting mic task on Core 0...");
    xTaskCreatePinnedToCore(micTask, "micTask", 4096, nullptr, 10, nullptr, 0);

    Serial.println("Step 3: Starting DAC task on Core 1...");
    xTaskCreatePinnedToCore(dacTask, "dacTask", 2048, nullptr, 24, nullptr, 1);

    Serial.println("=== Setup complete — speak into mic! ===");
}

void loop() {
    vTaskSuspend(nullptr);
}