#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// NODE config
#define SENDER_ID 0x02
#define NODE_NAME 'b'

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// Interference Testing Mode
#define TEST_PATTERN        0xAB
#define RF_TEST_TX          0
#define RF_TEST_PACKET_SIZE 32
#define RF_TEST_DELAY_MS    1000

// NRF24L01 config
#define NRF24L01_CE_PIN      4
#define NRF24L01_CSN_PIN     5
#define NRF24L01_POWER_LEVEL RF24_PA_MAX
#define NRF24L01_DATA_RATE   RF24_250KBPS

// Chat Config
#define MAX_INPUT_LENGTH 25
#define LOG_SIZE         10
#define MSG_MAX_TEXT     22

// Encryption Config
#define XOR_KEY     {0xAA, 0x3F, 0x12, 0x55}
#define XOR_KEY_LEN 4

// Input mode
#define USE_BUTTONS 0

#if defined(USE_BUTTONS) && USE_BUTTONS
#define BTN_UP_PIN     32
#define BTN_DOWN_PIN   33
#define BTN_SELECT_PIN 27
#define BTN_BACK_PIN   14
#else
#define JOYSTICK_Y_PIN    35
#define JOYSTICK_SW_PIN   33
#define JOYSTICK_DEADZONE 200
#define JOYSTICK_Y_CENTER 1730
#endif

// Screen config
#define I2C_ADDR       0x3C
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define SCREEN_SDA_PIN  21
#define SCREEN_SCL_PIN  22

// INMP441 I2S Microphone
#define I2S_PORT    I2S_NUM_0
#define I2S_SCK_PIN 13   // Serial clock
#define I2S_WS_PIN   16   // Word select
#define I2S_SD_PIN  17   // Serial data in
// L/R: wire to GND on module, no GPIO needed

// DAC output (receiver side only)
#define DAC_OUT_PIN 26   // ESP32 DAC2 — fixed by hardware

// Audio config
#define AUDIO_SAMPLE_RATE    8000  // Hz
#define AUDIO_PACKET_SAMPLES 28    // 1+1+2+28 = 32 bytes exactly
// 0 = disabled, 1 = TX (mic), 2 = RX (speaker), 3 = both if ever needed
#define AUDIO_ENABLED 1

#endif // CONFIG_H