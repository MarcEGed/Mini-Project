#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// Debug config
#define DEBUG 1
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

// NRF24L01 config
#define NRF24L01_CE_PIN 4
#define NRF24L01_CSN_PIN 5

// Screen config
// I2C addr 0x3D (for the 128x64)
#define I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA_PIN 21
#define SCREEN_SCL_PIN 22

// Rotary encoder config
#define ROTARY_ENCODER_CLK_PIN 32
#define ROTARY_ENCODER_DT_PIN 35
#define ROTARY_ENCODER_SW_PIN 33

// Optional compile-time ISR hooks for encoder direction events.
// Define these to function names available at compile time, for example:
#define ROTARY_ENCODER_CW_CALLBACK rotaryEncoderCW
#define ROTARY_ENCODER_CCW_CALLBACK rotaryEncoderCCW
#define ROTARY_ENCODER_SW_CALLBACK rotaryEncoderButtonPressed

#endif // CONFIG_H