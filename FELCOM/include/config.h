#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// NODE config
#ifndef FELCOM_VERSION
#define FELCOM_VERSION "unknown"
#endif
// change for each device flashed
#ifndef SENDER_ID
#define SENDER_ID 0x01
#endif
// in chat name, also change for each
#ifndef NODE_NAME
#define NODE_NAME 'b'
#endif

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// Interference Testing Mode
#define TEST_PATTERN        0xAB
#define RF_TEST_PACKET_SIZE 32
#define RF_TEST_DELAY_MS    1000

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
#define NRF24L01_MAX_CHANNEL_INDEX 38
const uint8_t USABLE_CHANNELS[NRF24L01_MAX_CHANNEL_INDEX + 1] = {
    13, 14, 15, 18, 19, 20, 23, 24, 25, 28, 29, 30, 33,
    34, 35, 38, 39, 40, 43, 44, 45, 48, 49, 50, 53, 54,
    55, 58, 59, 60, 63, 64, 65, 68, 69, 70, 77, 78, 79,
};

// Chat Config
// 1 byte space for termination character, might be needed
#define MAX_INPUT_LENGTH 25
// max messages kept
#define LOG_SIZE 10
// 1 (id) + 1 (name[2]) + 4 (ts) + 25 (text) + 1 spare = 32
#define MSG_MAX_TEXT 25

// Encryption Config
// same keys see the same stuff, different keys see encryption
#define XOR_KEY {0xAA, 0x3F, 0x12, 0x55}
#define XOR_KEY_LEN 4

// IMPORTANT: for debug purposes
// 1 for rotary encoder input
// 0 for joystick input
#define USE_BUTTONS 1

#if defined(USE_BUTTONS) && USE_BUTTONS
//button inputs
#define BTN_UP_PIN     32
#define BTN_DOWN_PIN   33
#define BTN_SELECT_PIN 27
#define BTN_BACK_PIN   14
#else

// Joystick config
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_SW_PIN 33
#define JOYSTICK_DEADZONE 200
#define JOYSTICK_Y_CENTER 1730
#endif

// Screen config
// I2C addr 0x3D (for the 128x64)
#define I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA_PIN 21
#define SCREEN_SCL_PIN 22

// Pong config
#define PONG_SCORE_TO_WIN 7
#define PONG_TICK_MS 30
#define PONG_TX_INTERVAL_MS 15

// Audio config
#define AUDIO_SAMPLE_RATE 8000

#define AUDIO_RING_SIZE 8192
#define AUDIO_PACKET_SIZE 32
#define AUDIO_PACKET_HEADER_BYTES 1

#define AUDIO_I2S_SCK_PIN 13
#define AUDIO_I2S_WS_PIN 16
#define AUDIO_I2S_SD_PIN 17

#define AUDIO_I2S_DMA_BUF_COUNT 8
#define AUDIO_I2S_DMA_BUF_LEN 512

#define AUDIO_MIC_GAIN 3
#define AUDIO_TX_PACING_US 200
#define AUDIO_DC_FILTER_ALPHA 63
#define AUDIO_DC_FILTER_DIV 64

#define AUDIO_DAC_CHANNEL DAC_CHANNEL_1
#define AUDIO_DAC_IDLE_VALUE 128

#endif  // CONFIG_H