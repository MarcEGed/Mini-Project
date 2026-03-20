#include <SPI.h>
#include "RF24.h"

#define CE_PIN 4
#define CSN_PIN 5
#define BUTTON_PIN 15   // push button pin

bool radioNumber = 0;
RF24 radio(CE_PIN, CSN_PIN);

byte addresses[][6] = { "1Node", "2Node" };

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // button to GND

  delay(2000);
  Serial.println("\nESP32 > NRF24L01 Button Transmitter");

  radio.begin();

  if (radio.isChipConnected())
    Serial.println("Transmitter NRF24 connected to SPI");
  else {
    Serial.println("NRF24 NOT connected");
    while (1);
  }

  radio.setChannel(125);
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_1MBPS);
  radio.powerUp();

  if (radioNumber) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  }

  radio.stopListening();
}

void loop() {

  bool buttonState = !digitalRead(BUTTON_PIN); 
  // pressed = 1, released = 0

  radio.write(&buttonState, sizeof(buttonState));

  Serial.print("Button state sent: ");
  Serial.println(buttonState);

  delay(50);
}