#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// ----- Matrix config -----
#define MATRIX_WIDTH 17
#define MATRIX_HEIGHT 9
#define LED_PIN 0

// ----- Button pins -----
#define LEFT_BTN   20
#define RIGHT_BTN  1
#define START_BTN  18

// ----- NeoMatrix object -----
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, LED_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS,
  NEO_GRB + NEO_KHZ800
);

// ----- Paddle -----
int paddleX = 7;
const int paddleWidth = 3;

// ----- Ball -----
int ballX = 8, ballY = 6;
int ballVX = 1, ballVY = 1;

// ----- Timing -----
unsigned long lastUpdate = 0;
const unsigned long FRAME_DELAY = 100;

bool gameOver = false;

// ----- Scroll state -----
int scrollX = MATRIX_WIDTH;
unsigned long lastScroll = 0;
const unsigned long SCROLL_DELAY = 100;

void setup() {
  matrix.begin();
  matrix.setBrightness(2);
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(255, 0, 0));
  matrix.fillScreen(0);
  matrix.show();

  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(START_BTN, INPUT_PULLUP);
}

void loop() {
  if (!gameOver) {
    if (millis() - lastUpdate > FRAME_DELAY) {
      moveBall();
      drawGame();
      lastUpdate = millis();
    }
    readInput();
  } else {
    scrollGameOver();
    if (digitalRead(START_BTN) == LOW) {
      delay(300);
      restartGame();
    }
  }
}

void readInput() {
  if (digitalRead(RIGHT_BTN) == LOW && paddleX > 0) {
    paddleX--;
    delay(50);
  }
  if (digitalRead(LEFT_BTN) == LOW && paddleX + paddleWidth < MATRIX_WIDTH) {
    paddleX++;
    delay(50);
  }
}

void moveBall() {
  ballX += ballVX;
  ballY += ballVY;

  // Bounce off left/right walls
  if (ballX <= 0 || ballX >= MATRIX_WIDTH - 1) ballVX *= -1;

  // Bounce off top wall
  if (ballY <= 0) ballVY *= -1;

  // Collision with paddle
  if (ballY == MATRIX_HEIGHT - 2) {
    if (ballX >= paddleX && ballX < paddleX + paddleWidth) {
      ballVY *= -1;
    } else {
      gameOver = true;
    }
  }

  // If ball falls below paddle
  if (ballY >= MATRIX_HEIGHT) {
    gameOver = true;
  }
}

void drawGame() {
  matrix.fillScreen(0);

  // Draw paddle
  for (int i = 0; i < paddleWidth; i++) {
    matrix.drawPixel(MATRIX_WIDTH - 1 - (paddleX + i), MATRIX_HEIGHT - 1, matrix.Color(0, 0, 255));
  }

  // Draw ball
  matrix.drawPixel(MATRIX_WIDTH - 1 - ballX, ballY, matrix.Color(255, 255, 0));

  matrix.show();
}

void scrollGameOver() {
  if (millis() - lastScroll > SCROLL_DELAY) {
    matrix.fillScreen(0);
    matrix.setCursor(scrollX, 1);
    matrix.print("GAME OVER");
    matrix.show();
    scrollX--;
    if (scrollX < -60) { // Reset scroll position once offscreen
      scrollX = MATRIX_WIDTH;
    }
    lastScroll = millis();
  }
}

void restartGame() {
  paddleX = 7;
  ballX = 8;
  ballY = 6;
  ballVX = 1;
  ballVY = 1;
  gameOver = false;
  scrollX = MATRIX_WIDTH;
  matrix.fillScreen(0);
  matrix.show();
}
