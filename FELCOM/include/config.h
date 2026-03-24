#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

//NODE config 
#define SENDER_ID 0x02 //change for each device flashed
#define NODE_NAME 'b'  //in chat name, also change for each

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// NRF24L01 config
#define NRF24L01_CE_PIN 4
#define NRF24L01_CSN_PIN 5

// Chat Config
#define MAX_INPUT_LENGTH 25 //1 byte space for termination character, might be needed
#define LOG_SIZE 10 //max messages kept
#define MSG_MAX_TEXT 25  //1 (id) + 1 (name[2]) + 4 (ts) + 25 (text) + 1 spare = 32

// Encryption Config
#define XOR_KEY {0xAA, 0x3F, 0x12, 0x55} //same keys see the same stuff, different keys see encryption
#define XOR_KEY_LEN 4

// IMPORTANT: for debug purposes
// 1 for rotary encoder input
// 0 for joystick input
#define USE_ROTARY_ENCODER 1

#if defined(USE_ROTARY_ENCODER) && USE_ROTARY_ENCODER
    // Rotary encoder config
    #define ROTARY_ENCODER_CLK_PIN 32
    #define ROTARY_ENCODER_DT_PIN 35
    #define ROTARY_ENCODER_SW_PIN 33

    // Optional compile-time ISR hooks for encoder direction events.
    // Define these to function names available at compile time, for example:
    #define ROTARY_ENCODER_CW_CALLBACK rotaryEncoderCW
    #define ROTARY_ENCODER_CCW_CALLBACK rotaryEncoderCCW
    #define ROTARY_ENCODER_SW_CALLBACK rotaryEncoderButtonPressed

#else
    // Joystick config
    #define JOYSTICK_Y_PIN  35
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


#endif // CONFIG_H