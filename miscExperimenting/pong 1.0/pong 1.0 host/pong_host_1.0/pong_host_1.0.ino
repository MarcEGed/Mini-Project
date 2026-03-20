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

int ballVX = 2;
int ballVY = 1;

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

  radio.openWritingPipe(addressHost);
  radio.openReadingPipe(1, addressClient);

  radio.startListening();

  game.ballX = 64;
  game.ballY = 32;
  game.paddleLeft = 25;
  game.paddleRight = 25;
}

void updateBall() {

  game.ballX += ballVX;
  game.ballY += ballVY;

  if (game.ballY <= 2 || game.ballY >= 61)
    ballVY = -ballVY;

  if (game.ballX <= 3 &&
      game.ballY >= game.paddleLeft &&
      game.ballY <= game.paddleLeft + 15)
    ballVX = -ballVX;

  if (game.ballX >= 124 &&
      game.ballY >= game.paddleRight &&
      game.ballY <= game.paddleRight + 15)
    ballVX = -ballVX;

  if (game.ballX < 0 || game.ballX > 127) {
    game.ballX = 64;
    game.ballY = 32;
    ballVX = -ballVX;
  }
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

  // move host paddle
  if (digitalRead(BTN_UP) == LOW) game.paddleLeft -= 2;
  if (digitalRead(BTN_DOWN) == LOW) game.paddleLeft += 2;

  game.paddleLeft = constrain(game.paddleLeft,2,46);

  // receive client paddle
  while (radio.available()) {
    radio.read(&game.paddleRight, sizeof(game.paddleRight));
  }

  updateBall();

  // send game state
  radio.stopListening();
  radio.write(&game, sizeof(game));
  radio.startListening();

  drawGame();

  delay(15);
}