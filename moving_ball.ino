#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define MATRIX_WIDTH 17
#define MATRIX_HEIGHT 9
#define PIN 0
#define MATRIX_OPTIONS (NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS)

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, PIN, MATRIX_OPTIONS, NEO_GRB + NEO_KHZ800
);

Adafruit_MPU6050 mpu;

// Ball state
float ballX = MATRIX_WIDTH / 2;
float ballY = MATRIX_HEIGHT / 2;
float velocityX = 0;
float velocityY = 0;

unsigned long lastUpdate = 0;
const unsigned long interval = 20;

// Physics parameters
const float friction = 0.98;     // slows down over time
const float accelScale = 0.05;   // how much accelerometer affects velocity

void setup() {
  matrix.begin();
  matrix.setBrightness(3);
  matrix.fillScreen(0);
  matrix.show();

  Wire.begin();
  if (!mpu.begin()) {
    while (1);  // Halt if MPU not found
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate < interval) return;
  lastUpdate = now;

  // Read MPU6050 accelerometer
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Adjust for card orientation
  float ax = -a.acceleration.y;  // left/right
  float ay = -a.acceleration.x;  // up/down

  // Add acceleration to velocity
  velocityX += ax * accelScale;
  velocityY += ay * accelScale;

  // Apply friction
  velocityX *= friction;
  velocityY *= friction;

  // Update position
  ballX += velocityX;
  ballY += velocityY;

  // Bounce off walls (considering ball is 2x2)
  if (ballX < 0) {
    ballX = 0;
    velocityX *= -1;
  } else if (ballX > MATRIX_WIDTH - 2) {
    ballX = MATRIX_WIDTH - 2;
    velocityX *= -1;
  }

  if (ballY < 0) {
    ballY = 0;
    velocityY *= -1;
  } else if (ballY > MATRIX_HEIGHT - 2) {
    ballY = MATRIX_HEIGHT - 2;
    velocityY *= -1;
  }

  // Draw bigger ball (2x2 pixels)
  matrix.fillScreen(0);
  int x = round(ballX);
  int y = round(ballY);
  uint16_t color = matrix.Color(255, 100, 0);

  for (int dx = 0; dx <= 1; dx++) {
    for (int dy = 0; dy <= 1; dy++) {
      int px = x + dx;
      int py = y + dy;
      if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
        matrix.drawPixel(px, py, color);
      }
    }
  }

  matrix.show();
}
