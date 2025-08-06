#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>

#define MATRIX_WIDTH 17
#define MATRIX_HEIGHT 9
#define NUMPIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define PIN 0

#define BTN_UP     19
#define BTN_DOWN   2
#define BTN_LEFT   20
#define BTN_RIGHT  1
#define BTN_CENTER 18

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS,
  NEO_GRB + NEO_KHZ800
);

// 9x9 Smiley face - open eyes
const uint8_t smiley_open[9][9] = {
  {0,0,1,1,1,1,1,0,0},
  {0,1,1,1,1,1,1,1,0},
  {1,1,1,2,1,2,1,1,1},
  {1,1,1,1,1,1,1,1,1},
  {1,1,3,1,1,1,3,1,1},
  {1,3,3,3,3,3,3,3,1},
  {1,1,1,3,3,3,1,1,1},
  {0,1,1,1,1,1,1,1,0},
  {0,0,1,1,1,1,1,0,0}
};

// Smiley with eyes closed
const uint8_t smiley_closed[9][9] = {
  {0,0,1,1,1,1,1,0,0},
  {0,1,1,1,1,1,1,1,0},
  {1,1,1,0,1,0,1,1,1}, // eyes "off"
  {1,1,1,1,1,1,1,1,1},
  {1,1,3,1,1,1,3,1,1},
  {1,3,3,3,3,3,3,3,1},
  {1,1,1,3,3,3,1,1,1},
  {0,1,1,1,1,1,1,1,0},
  {0,0,1,1,1,1,1,0,0}
};

uint16_t pixColor(uint8_t c) {
  switch (c) {
    case 1: return matrix.Color(255, 215, 0);   // Face
    case 2: return matrix.Color(0, 0, 255);     // Eyes (blue)
    case 3: return matrix.Color(255, 80, 0);    // Smile
    default: return 0;
  }
}

// Draw "Hi!"
void drawHiText() {
  uint16_t white = matrix.Color(255, 255, 255);

  // "H" at x=0
  matrix.drawPixel(0, 1, white);
  matrix.drawPixel(0, 2, white);
  matrix.drawPixel(0, 3, white);
  matrix.drawPixel(1, 2, white);
  matrix.drawPixel(2, 1, white);
  matrix.drawPixel(2, 2, white);
  matrix.drawPixel(2, 3, white);

  // Fixed "i" at x=4
  matrix.drawPixel(4, 0, white);  // dot
  matrix.drawPixel(4, 2, white);
  matrix.drawPixel(4, 3, white);
}

// Show text + smiley (open or closed)
void showHiWithSmiley(const uint8_t face[9][9]) {
  matrix.fillScreen(0);
  drawHiText();

  int offsetX = MATRIX_WIDTH - 9;
  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 9; col++) {
      uint8_t val = face[row][col];
      if (val > 0) {
        matrix.drawPixel(offsetX + col, row, pixColor(val));
      }
    }
  }
  matrix.show();
}

unsigned long lastBlink = 0;
bool eyesOpen = true;
unsigned long startTime;
enum AppState { MENU, SNAKE };
AppState currentState = MENU;

const char* menuItems[] = { "Snake", "PingPong", "Ball", "Weather" };
const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
int selectedIndex = 0;
int scrollX = MATRIX_WIDTH;
unsigned long lastScroll = 0;
const int scrollSpeed = 100;

enum Direction { UP, DOWN, LEFT, RIGHT };
Direction dir = RIGHT;
bool updateDir = false;

struct Point { int x, y; };
#define MAX_SNAKE_LEN NUMPIXELS
Point snake[MAX_SNAKE_LEN];
int snakeLen = 3;
Point food;
bool gameOver = false;
int score = 0;
int kl=0;
unsigned long lastMove = 0;
const unsigned long MOVE_INTERVAL = 150;

void setup() {
  matrix.begin();
  matrix.setBrightness(3);
  matrix.fillScreen(0);
  matrix.show();
Serial.begin(9600);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_CENTER, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_UP),    upISR,    FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_DOWN),  downISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_LEFT),  leftISR,  FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_RIGHT), rightISR, FALLING);
 showHiWithSmiley(smiley_open);
 startTime = millis();
  //resetGame();
}

void loop() {
    if (millis() - startTime < 5000) {
    if (millis() - lastBlink >= 1000) {
      eyesOpen = !eyesOpen;
      showHiWithSmiley(eyesOpen ? smiley_open : smiley_closed);
      lastBlink = millis();
    }
  } else {
  if (currentState == MENU) {
    handleMenuButtons();
    drawScrollingMenu(menuItems[selectedIndex]);

    if (millis() - lastScroll > scrollSpeed) {
      scrollX--;
      int textWidth = strlen(menuItems[selectedIndex]) * 6;
      if (scrollX < -textWidth) scrollX = MATRIX_WIDTH;
      lastScroll = millis();
    }

    if (digitalRead(BTN_CENTER) == LOW) {
      delay(200);
      launchSelectedMode();
    }
  } else if (currentState == SNAKE) {
    runSnakeGame();
  }
}
}

void handleMenuButtons() {
  static unsigned long lastBtn = 0;
  if (millis() - lastBtn < 200) return;

  if (digitalRead(BTN_LEFT) == LOW) {
    selectedIndex = (selectedIndex - 1 + menuCount) % menuCount;
    scrollX = MATRIX_WIDTH;
    lastBtn = millis();
  } else if (digitalRead(BTN_RIGHT) == LOW) {
    selectedIndex = (selectedIndex + 1) % menuCount;
    scrollX = MATRIX_WIDTH;
    lastBtn = millis();
  }
}

void drawScrollingMenu(const char* text) {
  matrix.fillScreen(0);
  matrix.setCursor(scrollX, 0);
  matrix.setTextColor(matrix.Color(0, 255, 0));
  matrix.setTextWrap(false);
  matrix.print(text);
  matrix.show();
}

void launchSelectedMode() {
  const char* item = menuItems[selectedIndex];
  if (strcmp(item, "Snake") == 0) {
    currentState = SNAKE;
    resetGame();
  }
}

void runSnakeGame() {
  if (!gameOver) {
    if (millis() - lastMove > MOVE_INTERVAL) {
      moveSnake();
      drawSnakeMatrix();
      lastMove = millis();
      updateDir = false;
    }
  } else {
    showScore();
    if (digitalRead(BTN_CENTER) == LOW) {
      delay(300);
      currentState = MENU;
      scrollX = MATRIX_WIDTH;
    }
  }
}

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

void moveSnake() {
  Point lastTail = snake[snakeLen - 1];
  for (int i = snakeLen - 1; i > 0; i--) snake[i] = snake[i - 1];

  switch (dir) {
    case UP:    snake[0].y--; break;
    case DOWN:  snake[0].y++; break;
    case LEFT:  snake[0].x--; break;
    case RIGHT: snake[0].x++; break;
  }

  if (snake[0].x < 0) snake[0].x = MATRIX_WIDTH - 1;
  if (snake[0].x >= MATRIX_WIDTH) snake[0].x = 0;
  if (snake[0].y < 0) snake[0].y = MATRIX_HEIGHT - 1;
  if (snake[0].y >= MATRIX_HEIGHT) snake[0].y = 0;

  for (int i = 1; i < snakeLen; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true;
      return;
    }
  }

  if (snake[0].x == food.x && snake[0].y == food.y) {
    if (snakeLen < MAX_SNAKE_LEN) {
      snake[snakeLen] = lastTail;
      snakeLen++;
    }
    score++;
    placeFood();
  }
}

void drawSnakeMatrix() {
  matrix.fillScreen(0);
  matrix.drawPixel(food.x, food.y, matrix.Color(255, 0, 0));
  for (int i = 0; i < snakeLen; i++) {
    uint16_t color = (i == 0) ? matrix.Color(0, 255, 0) : matrix.Color(0, 0, 255);
    matrix.drawPixel(snake[i].x, snake[i].y, color);
  }
  matrix.show();
}

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

void showScore() {
  matrix.fillScreen(0);
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(255, 255, 0));
  matrix.setCursor(3, 0);
  matrix.print(score);
  matrix.show();
}

void upISR()    { if (!updateDir && dir != DOWN)  { dir = UP;    updateDir = true; } }
void downISR()  { if (!updateDir && dir != UP)    { dir = DOWN;  updateDir = true; } }
void leftISR()  { if (!updateDir && dir != RIGHT) { dir = LEFT;  updateDir = true; } }
void rightISR() { if (!updateDir && dir != LEFT)  { dir = RIGHT; updateDir = true; } }