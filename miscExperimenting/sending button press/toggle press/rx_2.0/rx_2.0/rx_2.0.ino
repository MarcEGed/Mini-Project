#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5
#define LED_PIN 2

bool radioNumber = 1;
const uint8_t pipes[][6] = { "1Node", "2Node" };

RF24 radio(CE_PIN, CSN_PIN);

bool command;
bool ledState = false;

void setup() {

  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  delay(1000);
  Serial.println("ESP32 > NRF24 Toggle Receiver");

  radio.begin();

  if (radio.isChipConnected())
    Serial.println("Receiver NRF24 connected to SPI");
  else {
    Serial.println("NRF24 NOT connected");
    while (1);
  }

  radio.setChannel(125);
  radio.setDataRate(RF24_1MBPS);

  if (!radioNumber) {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
  } else {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
  }

  radio.startListening();
}

void loop() {

  if (radio.available()) {

    radio.read(&command, sizeof(command));

    // toggle LED
    ledState = !ledState;

    digitalWrite(LED_PIN, ledState);

    Serial.print("LED State: ");
    Serial.println(ledState);
  }
}