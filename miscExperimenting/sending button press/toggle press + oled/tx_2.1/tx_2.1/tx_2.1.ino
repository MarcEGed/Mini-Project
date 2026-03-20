#include <SPI.h>
#include "RF24.h"

#define CE_PIN 4
#define CSN_PIN 5
#define BUTTON_PIN 15

bool radioNumber = 0;
RF24 radio(CE_PIN, CSN_PIN);

byte addresses[][6] = { "1Node", "2Node" };

bool lastButtonState = HIGH;

void setup() {

  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  delay(2000);
  Serial.println("\nESP32 > NRF24L01 Toggle Transmitter");

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

  bool buttonState = digitalRead(BUTTON_PIN);

  // detect button press
  if (buttonState == LOW && lastButtonState == HIGH) {

    bool command = true;   // toggle command

    radio.write(&command, sizeof(command));

    Serial.println("Button pressed -> toggle command sent");

    delay(200); // simple debounce
  }

  lastButtonState = buttonState;
}