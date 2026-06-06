#include <Arduino.h>
#include "driver/i2s.h"
#include <SPI.h>
#include <RF24.h>

// ── NRF ─────────────────────────────────────
RF24 radio(4, 5); // CE, CSN
const byte address[6] = "AUDIO";

// ── I2S Mic Pins ────────────────────────────
#define I2S_SCK   13
#define I2S_WS    16
#define I2S_SD    17

// ── Settings ────────────────────────────────
#define SAMPLE_RATE     8000   // IMPORTANT: reduced
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     128
#define GAIN            24

static int32_t i2sBuf[DMA_BUF_LEN];
static int32_t dc_offset = 0;

// ── Mic Setup ───────────────────────────────
void setupMicI2S() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.dma_buf_count = DMA_BUF_COUNT;
    cfg.dma_buf_len = DMA_BUF_LEN;

    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);

    i2s_pin_config_t pins = {};
    pins.bck_io_num = I2S_SCK;
    pins.ws_io_num = I2S_WS;
    pins.data_in_num = I2S_SD;
    pins.data_out_num = I2S_PIN_NO_CHANGE;

    i2s_set_pin(I2S_NUM_0, &pins);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

// ── Mic Task (STREAMING) ────────────────────
void micTask(void* param) {
    uint8_t packet[32];
    uint8_t seq = 0;
    uint8_t idx = 1;

    while (true) {
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, i2sBuf, sizeof(i2sBuf), &bytesRead, portMAX_DELAY);
        uint32_t count = bytesRead / 4;

        for (uint32_t i = 0; i < count; i++) {
            int32_t sample24 = i2sBuf[i] >> 8;

            dc_offset = (dc_offset * 63 + sample24) / 64;
            int32_t centered = sample24 - dc_offset;

            int32_t scaled = (centered * GAIN) >> 16;
            int32_t val = 128 + scaled;

            if (val > 255) val = 255;
            if (val < 0) val = 0;

            packet[idx++] = (uint8_t)val;

            if (idx == 32) {
                packet[0] = seq++;
                radio.write(packet, 32);
                idx = 1;

                delayMicroseconds(200); // pacing
            }
        }
    }
    vTaskDelay(1);
}

// ── Setup ───────────────────────────────────
void setup() {
    Serial.begin(115200);

    setupMicI2S();

    radio.begin();
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.setRetries(3, 5);
    radio.openWritingPipe(address);
    radio.stopListening();

    xTaskCreatePinnedToCore(micTask, "micTask", 4096, NULL, 2, NULL, 0);
}

void loop() {
    vTaskSuspend(NULL);
}