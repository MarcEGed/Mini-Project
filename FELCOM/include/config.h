#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// NODE config
#ifndef FELCOM_VERSION
#define FELCOM_VERSION "unknown"
#endif
// change for each device flashed
#ifndef NODE_ID
#define NODE_ID 'B'
#endif

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// Interference Testing Mode
#define TEST_PATTERN 0xAB
#define RF_TEST_PACKET_SIZE 28
#define RF_TEST_DELAY_MS 1000

// NRF24L01 config
#define NRF24L01_CE_PIN 4
#define NRF24L01_CSN_PIN 5
#define NRF24L01_POWER_LEVEL RF24_PA_MAX
#define NRF24L01_DATA_RATE RF24_1MBPS
// nodes needs to be on the same PHY to talk to each other
// MAX 5
#define NRF24L01_PHY_ADDR 0
// ADDR 0 and 1 will store a full 5-byte address.
// ADDR 2-5 will technically only store a single byte, borrowing up to 4
// additional bytes from pipe 1 per the assigned address width
const uint8_t PHY_ADDRESSES[6][6] = {"FELCO", "GELCO", "HELCO",
                                     "JELCO", "KELCO", "LELCO"};

#define HOPPING_CHANNELS_SIZE 6
const uint8_t HOPPING_CHANNELS[HOPPING_CHANNELS_SIZE] = {110, 111, 112,
                                                         113, 114, 115};

// FHSS join/sync config
<<<<<<< Updated upstream
#define FHSS_BOOTSTRAP_TIMEOUT_MS 12
#define FHSS_BOOTSTRAP_RETRY_MS 2
=======
#define FHSS_BOOTSTRAP_TIMEOUT_MS 2000  // 2s: CSMA can take 30ms/attempt, 12ms was too short
#define FHSS_BOOTSTRAP_RETRY_MS 50
>>>>>>> Stashed changes
#define FHSS_TIMER_PERIOD_US 200000  // 200ms hop period
#define FHSS_CSMA_BACKOFF_MIN_US 1000
#define FHSS_CSMA_BACKOFF_MAX_US 10000
#define FHSS_CSMA_ATTEMPTS 3
#define FHSS_OUT_OF_SYNC_HOPS 1000
<<<<<<< Updated upstream
=======
// Max timer delta from a known peer that triggers drift correction (~10% of hop period)
#define FHSS_DRIFT_CORRECTION_US 20000
>>>>>>> Stashed changes

// Chat Config
#define MAX_INPUT_LENGTH 25
#define LOG_SIZE 10
#define MSG_MAX_TEXT 22

// Encryption Config
#define XOR_KEY {0xAA, 0x3F, 0x12, 0x55}
#define XOR_KEY_LEN 4

// IMPORTANT: for debug purposes
// 1 for rotary encoder input
// 0 for joystick input
#define USE_BUTTONS 1

#if defined(USE_BUTTONS) && USE_BUTTONS
// button inputs
#define BTN_UP_PIN 32
#define BTN_DOWN_PIN 33
#define BTN_SELECT_PIN 27
#define BTN_BACK_PIN 14
#else

// Joystick config
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_SW_PIN 33
#define JOYSTICK_DEADZONE 200
#define JOYSTICK_Y_CENTER 1730
#endif

// Screen config
#define I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA_PIN 21
#define SCREEN_SCL_PIN 22

// INMP441 I2S Microphone
#define I2S_PORT I2S_NUM_0
#define I2S_SCK_PIN 13  // Serial clock
#define I2S_WS_PIN 16   // Word select
#define I2S_SD_PIN 17   // Serial data in
// L/R: wire to GND on module, no GPIO needed

// DAC output (receiver side only)
#define DAC_OUT_PIN 26  // ESP32 DAC2 — fixed by hardware

// Audio config
#define AUDIO_SAMPLE_RATE 8000   // Hz
#define AUDIO_PACKET_SAMPLES 28  // 1+1+2+28 = 32 bytes exactly
// 0 = disabled, 1 = TX (mic), 2 = RX (speaker), 3 = both if ever needed
#define AUDIO_ENABLED 1

// Pong config
#define PONG_SCORE_TO_WIN 7
#define PONG_TICK_MS 30
#define PONG_TX_INTERVAL_MS 15

#endif  // CONFIG_H
