#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// NODE config
// change for each device flashed
#define SENDER_ID 0x01
// in chat name, also change for each
#define NODE_NAME 'a'

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// Interference Testing Mode
#define TEST_PATTERN        0xAB
#define RF_TEST_TX          1
#define RF_TEST_PACKET_SIZE 32
#define RF_TEST_DELAY_MS    1000

// NRF24L01 config
#define NRF24L01_CE_PIN 4
#define NRF24L01_CSN_PIN 5
#define NRF24L01_POWER_LEVEL RF24_PA_MAX
#define NRF24L01_DATA_RATE RF24_250KBPS

// Chat Config
// 1 byte space for termination character, might be needed
#define MAX_INPUT_LENGTH 25
// max messages kept
#define LOG_SIZE 10
// 1 (id) + 1 (name[2]) + 4 (ts) + 25 (text) + 1 spare = 32
#define MSG_MAX_TEXT 22

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
#define BTN_UP_PIN     26
#define BTN_DOWN_PIN   25
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

#endif  // CONFIG_H