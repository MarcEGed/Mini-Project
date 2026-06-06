#include <Arduino.h>
#include "driver/dac.h"
#include <SPI.h>
#include <RF24.h>

// ── NRF ─────────────────────────────────────
RF24 radio(4, 5);
const byte address[6] = "AUDIO";

// ── Settings ────────────────────────────────
#define SAMPLE_RATE 8000

// ── Ring Buffer ─────────────────────────────
#define RING_SIZE  8192
#define RING_MASK  (RING_SIZE - 1)

static volatile uint8_t ring[RING_SIZE];
static volatile uint32_t ringHead = 0;
static volatile uint32_t ringTail = 0;

// ── NRF Task ────────────────────────────────
void nrfTask(void* param) {

    uint8_t packet[32];

    while (true) {

        while (radio.available()) {

            radio.read(packet, 32);

            for (int i = 1; i < 32; i++) {

                uint32_t nextHead = (ringHead + 1) & RING_MASK;

                if (nextHead != ringTail) {
                    ring[ringHead] = packet[i];
                    ringHead = nextHead;
                }
            }
        }

        vTaskDelay(1);
    }
}

// ── DAC Task ────────────────────────────────
void dacTask(void* param) {

    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 128);

    const uint32_t sampleDelayUs = 125; // 8000Hz

    while (true) {

        if (ringTail != ringHead) {
            dac_output_voltage(DAC_CHANNEL_1, ring[ringTail]);
            ringTail = (ringTail + 1) & RING_MASK;
        }
        else {
            dac_output_voltage(DAC_CHANNEL_1, 128);
        }

        ets_delay_us(sampleDelayUs);

        // VERY IMPORTANT
        taskYIELD();
    }
}

// ── Setup ───────────────────────────────────
void setup() {
    Serial.begin(115200);
    printResetReason();
    radio.begin();
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.openReadingPipe(0, address);
    radio.startListening();

    xTaskCreatePinnedToCore(nrfTask, "nrfTask", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(dacTask, "dacTask", 2048, NULL, 3, NULL, 1);
}

void loop() {
    vTaskSuspend(NULL);
}

#include "esp_system.h"

void printResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();

    Serial.print("Reset reason: ");

    switch(reason) {
        case ESP_RST_POWERON: Serial.println("Power on"); break;
        case ESP_RST_SW: Serial.println("Software reset"); break;
        case ESP_RST_PANIC: Serial.println("Exception/panic"); break;
        case ESP_RST_INT_WDT: Serial.println("Interrupt watchdog"); break;
        case ESP_RST_TASK_WDT: Serial.println("Task watchdog"); break;
        case ESP_RST_WDT: Serial.println("Other watchdog"); break;
        case ESP_RST_BROWNOUT: Serial.println("Brownout"); break;
        default: Serial.println(reason); break;
    }
}