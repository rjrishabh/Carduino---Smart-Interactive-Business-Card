#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>

// ----- MATRIX CONFIG -----
#define MATRIX_WIDTH 17
#define MATRIX_HEIGHT 9
#define NUMPIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define PIN 0

// ----- BUTTON PINS -----
#define UP_BTN     19
#define DOWN_BTN   2
#define LEFT_BTN   20
#define RIGHT_BTN  1
#define START_BTN  18

// ----- MATRIX OBJECT (No serpentine, top to bottom per column) -----
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS,
  NEO_GRB + NEO_KHZ800
);

// ----- GAME ENUMS -----
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction dir = RIGHT;
bool updateDir = false;

// ----- SNAKE STRUCTURE -----
struct Point { int x, y; };
#define MAX_SNAKE_LEN NUMPIXELS
Point snake[MAX_SNAKE_LEN];
int snakeLen = 3;
Point food;
bool gameOver = false;
int score = 0;

// ----- TIMING -----
unsigned long lastMove = 0;
const unsigned long MOVE_INTERVAL = 150;

// ----- SETUP -----
void setup() {
  matrix.begin();
  matrix.setBrightness(3);
  matrix.fillScreen(0);
  matrix.show();

  pinMode(UP_BTN, INPUT_PULLUP);
  pinMode(DOWN_BTN, INPUT_PULLUP);
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(START_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(UP_BTN),    upISR,    FALLING);
  attachInterrupt(digitalPinToInterrupt(DOWN_BTN),  downISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(LEFT_BTN),  leftISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_BTN), rightISR, FALLING);

  resetGame();
}

// ----- MAIN LOOP -----
void loop() {
  if (!gameOver) {
    if (millis() - lastMove > MOVE_INTERVAL) {
      moveSnake();
      drawMatrix();
      lastMove = millis();
      updateDir = false;
    }
  } else {
    showScore();

    // Wait for START button to restart
    if (digitalRead(START_BTN) == LOW) {
      delay(300);  // Debounce
      resetGame();
      gameOver = false;
    }
  }
}

// ----- RESET GAME -----
void resetGame() {
  snakeLen = 3;
  dir = RIGHT;
  updateDir = false;
  score = 0;
  snake[0] = {4, 4};
  snake[1] = {3, 4};
  snake[2] = {2, 4};
  placeFood();
  matrix.fillScreen(0);
  matrix.show();
}

// ----- MOVE SNAKE -----
void moveSnake() {
  // Save last tail position
  Point lastTail = snake[snakeLen - 1];

  // Shift body
  for (int i = snakeLen - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  // Move head
  switch (dir) {
    case UP:    snake[0].y--; break;
    case DOWN:  snake[0].y++; break;
    case LEFT:  snake[0].x--; break;
    case RIGHT: snake[0].x++; break;
  }

  // Wrap around
  if (snake[0].x < 0) snake[0].x = MATRIX_WIDTH - 1;
  if (snake[0].x >= MATRIX_WIDTH) snake[0].x = 0;
  if (snake[0].y < 0) snake[0].y = MATRIX_HEIGHT - 1;
  if (snake[0].y >= MATRIX_HEIGHT) snake[0].y = 0;

  // Self-collision
  for (int i = 1; i < snakeLen; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true;
      return;
    }
  }

  // Food collision
  if (snake[0].x == food.x && snake[0].y == food.y) {
    if (snakeLen < MAX_SNAKE_LEN) {
      // Add new segment at previous tail
      snake[snakeLen] = lastTail;
      snakeLen++;
    }
    score++;
    placeFood();
  }
}


// ----- DRAW MATRIX -----
void drawMatrix() {
  matrix.fillScreen(0);

  // Draw food
  matrix.drawPixel(food.x, food.y, matrix.Color(255, 0, 0));

  // Draw snake
  for (int i = 0; i < snakeLen; i++) {
    uint16_t color = (i == 0) ? matrix.Color(0, 255, 0) : matrix.Color(0, 0, 255);
    matrix.drawPixel(snake[i].x, snake[i].y, color);
  }

  matrix.show();
}

// ----- PLACE FOOD -----
void placeFood() {
  while (true) {
    int x = random(MATRIX_WIDTH);
    int y = random(MATRIX_HEIGHT);

    bool onSnake = false;
    for (int i = 0; i < snakeLen; i++) {
      if (snake[i].x == x && snake[i].y == y) {
        onSnake = true;
        break;
      }
    }

    if (!onSnake) {
      food = {x, y};
      return;
    }
  }
}

// ----- SHOW SCORE -----
void showScore() {
  matrix.fillScreen(0);
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(255, 255, 0));
  matrix.setCursor(3, 0);
  matrix.print(score);
  matrix.show();
}

// ----- BUTTON INTERRUPTS -----
void upISR()    { if (!updateDir && dir != DOWN)  { dir = UP;    updateDir = true; } }
void downISR()  { if (!updateDir && dir != UP)    { dir = DOWN;  updateDir = true; } }
void leftISR()  { if (!updateDir && dir != RIGHT) { dir = LEFT;  updateDir = true; } }
void rightISR() { if (!updateDir && dir != LEFT)  { dir = RIGHT; updateDir = true; } }
