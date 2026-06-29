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

// Chat Config
// Shrunk from 25/22 so the chat wire frame (2B seq + msg + 2B CRC) fits the
// 28-byte data[] region once the FEC layer reserves seq + CRC. See fec.md.
#define MAX_INPUT_LENGTH 18
#define LOG_SIZE 10
#define MSG_MAX_TEXT 18

// Encryption Config
#define XOR_KEY {0xAA, 0x3F, 0x12, 0x55}
#define XOR_KEY_LEN 4

// button inputs
#define BTN_UP_PIN 32
#define BTN_DOWN_PIN 33
#define BTN_SELECT_PIN 27
#define BTN_BACK_PIN 14


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
#define DAC_OUT_PIN 25  // ESP32 DAC1 (GPIO25) — matches the audio amp wiring

// Audio config
#define AUDIO_SAMPLE_RATE 8000   // Hz
#define AUDIO_PACKET_SAMPLES 24  // seq(2) + samples(24) = 26 user bytes (FEC_AUDIO_SAMPLES)
// 0 = disabled, 1 = TX (mic), 2 = RX (speaker), 3 = both if ever needed
#define AUDIO_ENABLED 1
// INMP441 L/R tied to GND selects the left I2S slot. With ESP32 legacy I2S in
// RIGHT_LEFT mode, boards/libraries can disagree on whether that arrives as
// buffer index 0 or 1. If mic diagnostics show near-zero level, try changing
// this between 0 and 1 before changing gain.
#define AUDIO_I2S_SLOT_INDEX 0
// Software mic gain: scaled = (centered * AUDIO_GAIN) >> 16.
//   higher = louder but more clipping/hiss; lower = cleaner but quieter.
//   Tune by ear: try 12 / 18 / 24 / 32.
#define AUDIO_GAIN 8
// Set to 1 to hear locally the same 8-bit mic samples being transmitted while
// TALKING. Useful for checking mic/I2S/gain before debugging the radio link.
#define AUDIO_LOCAL_MONITOR 0
// Max local monitor backlog while TALKING. Lower = less sidetone delay; if too
// low it can sound rough because old samples are dropped to stay live.
#define AUDIO_LOCAL_MONITOR_MAX_QUEUED 24
// Set to 0 to test only the mic -> local DAC path without nRF24/FEC blocking.
// Leave 1 for normal walkie-talkie TX.
#define AUDIO_RADIO_TX_ENABLE 1
// Set to 1 for serial audio diagnostics. Keep 0 for normal playback: serial
// printing stalls the cooperative audio loop and causes audible glitches.
#define AUDIO_SERIAL_DEBUG 0
// DAC playback ring (power of two; smooths radio jitter in LISTEN and paces the
// local mic monitor in TALK).
#define AUDIO_RX_RING 4096

// FEC config — see "FEC Prototype — Design Document" (fec.md)
#define FEC_XOR_BLOCK_SIZE 4    // data packets per XOR block (+1 parity packet)
#define FEC_ARQ_TIMEOUT_MS 200  // wait for ACK before retransmit
#define FEC_ARQ_MAX_RETRIES 3   // max ARQ retransmissions
#define FEC_AUDIO_RX_QUEUE 8    // recovered audio payloads buffered for playback

// Pong config
#define PONG_SCORE_TO_WIN 7
#define PONG_TICK_MS 30
#define PONG_TX_INTERVAL_MS 15

#endif  // CONFIG_H
