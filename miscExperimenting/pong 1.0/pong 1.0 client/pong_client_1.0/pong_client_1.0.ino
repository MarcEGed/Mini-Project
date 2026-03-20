#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CE_PIN 4
#define CSN_PIN 5

#define BTN_UP 12
#define BTN_DOWN 13

RF24 radio(CE_PIN, CSN_PIN);

const byte addressHost[6] = "HOST1";
const byte addressClient[6] = "CLNT1";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct GamePacket {
  int ballX;
  int ballY;
  int paddleLeft;
  int paddleRight;
};

GamePacket game;

int paddleRight = 25;

void setup() {

  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  radio.begin();
  if (!radio.isChipConnected()) {
    Serial.println("NRF24 not responding!");
    while (1);
  }
  radio.setChannel(125);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);

  radio.openWritingPipe(addressClient);
  radio.openReadingPipe(1, addressHost);

  radio.startListening();

  game.ballX = 64;
  game.ballY = 32;
  game.paddleLeft = 25;
  game.paddleRight = 25;
}

void drawGame() {

  display.clearDisplay();

  display.drawLine(0,0,127,0,1);
  display.drawLine(0,63,127,63,1);

  display.drawLine(64,2,64,61,1);

  display.drawLine(1,game.paddleLeft,1,game.paddleLeft+15,1);
  display.drawLine(126,game.paddleRight,126,game.paddleRight+15,1);

  display.drawCircle(game.ballX,game.ballY,1,1);

  display.display();
}

void loop() {

  // move paddle locally
  if (digitalRead(BTN_UP) == LOW) paddleRight -= 2;
  if (digitalRead(BTN_DOWN) == LOW) paddleRight += 2;

  paddleRight = constrain(paddleRight,2,46);

  // send paddle to host
  radio.stopListening();
  radio.write(&paddleRight, sizeof(paddleRight));
  radio.startListening();

  delay(2);   // allow host time to respond

  // receive game state
  if (radio.available()) {
    radio.read(&game, sizeof(game));
  }

  drawGame();

  delay(15);
}