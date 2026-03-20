// esp32 > NRF24L01 transmitter test using a text string

// RP2040 connections
// RP2040 SPIO_SCK pin GP18 goes to NRF24L10_pin SCK
// RP2040 SPIO_RX pin GP16 goes to NRF24L10_pin MISO
// RP2040 SPIO_TX pin GP19 goes to NRF24L10_pin MOSI
// RP2040 pin SPIO_CSn GP17 to NRF24L10 CSN
// RP2040 pin GP20 to NRF24L10 CE
// RP2040 GND and 3.3V to NRF24L10  GND and VCC

// Leonardo connections ???
// Leonardo ICSP SCK  pin 15 to NRF24L10_pin SCK
// Leonardo ICSP MISO pin 14 to NRF24L10_pin MISO
// Leonardo ICSP MOSI pin 16 to NRF24L10_pin MOSI
// Leonardo pin 10 to NRF24L10 CSN
// Leonardo pin 9  to NRF24L10 CE
// Leonardo GND and 3.3V to NRF24L10  GND and VCC

// ESP8266 connections
// ESP8266 SCK pin GPIO14 goes to NRF24L10_pin SCK
// ESP8266 MISO pin GPIO12 goes to NRF24L10_pin MI
// ESP8266 MOSI pin GPIO13 goes to NRF24L10_pin MO
// NRF24L10 CE to ESP8266 pin GPIO4
// NRF24L10 CSN to ESP8266 pin GPIO5

// UNO/Nano connections
// arduino MOSI pin 11 goes to NRF24L10_pin MOSI
// arduino MISO pin 12 goes to NRF24L10_pin MI
// arduino SCK  pin 13 goes to NRF24L10_pin SCK
// NRF24L10 CE to arduino pin 9
// NRF24L10 CSN to arduino pin10
// NRF24L10 VCC to 3.3V and GND to GND

// for ESP32-CAM
//#define SCK  14
//#define MISO  12
//#define MOSI  13
//#define CS  15   

// ESP32 connections
// ESP32 SCK pin GPIO18 goes to NRF24L10_pin SCK
// ESP32 MISO pin GPIO19 goes to NRF24L10_pin MI
// ESP32 MOSI pin GPIO23 goes to NRF24L10_pin MO
// NRF24L10 CE to ESP32 pin GPIO4
// NRF24L10 CSN to ESP32 pin GPIO 5

#include <SPI.h>
#include "RF24.h"

#define CE_PIN 4
#define CSN_PIN 5

bool radioNumber = 0;
RF24 radio(CE_PIN, CSN_PIN);

byte addresses[][6] = { "1Node", "2Node" };

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nESP32> NRF24L01 transmit text");
  radio.begin();
  if (radio.isChipConnected())
    Serial.println("Transmitter NF24 connected to SPI");
  else {
    Serial.println("NF24 is NOT connected to SPI");
    while (1)
      ;
  }
  radio.setChannel(125);
  radio.setPALevel(RF24_PA_MIN);
  radio.powerUp();
  radio.setDataRate(RF24_1MBPS);
  //radio.setDataRate(RF24_250KBPS);
  if (radioNumber) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  }
  radio.stopListening();
  //radio.setPayloadSize(sizeof(Struct1));
}

// loop transmiting data packet
void loop() {
  static char testString[10] = "text 0";
  radio.write(testString, sizeof(testString));
  Serial.print("transmit ");
  Serial.println(testString);
  delay(1000);
  testString[5]++;
}