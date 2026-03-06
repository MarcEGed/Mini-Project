//  ESP32 > NRF24L01 receiver test using a text string

// UNO/Nano connections
// arduino SCK pin 11 goes to NRF24L10_pin MOSI
// arduino MISO pin 12 goes to NRF24L10_pin MI
// arduino MOSI pin 13 goes to NRF24L10_pin SCK
// NRF24L10 CE to arduino pin 9
// NRF24L10 CSN to arduino pin10

// ESP32 connections
// ESP32 SCK pin GPIO18 goes to NRF24L10_pin SCK
// ESP32 MISO pin GPIO19 goes to NRF24L10_pin MI
// ESP32 MOSI pin GPIO23 goes to NRF24L10_pin MO
// NRF24L10 CE to ESP32 pin GPIO4
// NRF24L10 CSN to ESP32 pin GPIO 5



#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5

bool radioNumber = 1;
const uint8_t pipes[][6] = { "1Node", "2Node" };

RF24 radio(CE_PIN, CSN_PIN);

char dataReceived[10];  // this must match dataToSend in the TX
bool newData = false;

//===========

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 > NRF24L01 Receive text");
  radio.begin();
  if (radio.isChipConnected())
    Serial.println("Receiver NF24 connected to SPI");
  else {
    Serial.println("NF24 is NOT connected to SPI");
    while (1)
      ;
  }
  radio.setChannel(125);
  radio.setDataRate(RF24_1MBPS);
  //radio.setDataRate(RF24_250KBPS);
  radio.printDetails();
  if (!radioNumber) {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
  } else {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
  }
  radio.startListening();
  // radio.setPayloadSize(sizeof(Struct1));
}

//=============

void loop() {
  if (radio.available()) {
    char testString[10] = "";
    radio.read(testString, sizeof(testString));
    Serial.print("Test string:      ");
    Serial.println(testString);
  }
}