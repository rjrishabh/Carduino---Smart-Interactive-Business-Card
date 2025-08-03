#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_NeoMatrix.h>    // For NeoPixel Matrix
#include <Adafruit_NeoPixel.h>     // Basic NeoPixel library
 
#define DATA_PIN 0              // Use GPIO 2 for the WS2812B Data line
#define MATRIX_WIDTH 17            // Width of the matrix (8x8)
#define MATRIX_HEIGHT 9           // Height of the matrix
 
// Initialize the matrix with different orientation
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, DATA_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +  // Change NEO_MATRIX_LEFT to NEO_MATRIX_RIGHT
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);
 
// Your scrolling message
char message[] = "Hello, World!";
int textPos = MATRIX_WIDTH;   // Start the text off-screen to the right
 
// Define some colors
uint16_t colors[] = {
  matrix.Color(255, 0, 0),    // Red
  matrix.Color(0, 255, 0),    // Green
  matrix.Color(0, 0, 255),    // Blue
  matrix.Color(255, 255, 0),  // Yellow
  matrix.Color(255, 0, 255),  // Magenta
  matrix.Color(0, 255, 255),  // Cyan
  matrix.Color(255, 165, 0),  // Orange
  matrix.Color(255, 255, 255) // White
};
 
void setup() {
  matrix.begin();
  matrix.setTextWrap(false);       // Disable text wrapping
  matrix.setBrightness(1);        // Set brightness (0-255)
}
 
void loop() {
  matrix.fillScreen(0);            // Clear the screen
  int pos = textPos;               // Temporary position for each character
 
  // Loop through each character in the message
  for (int i = 0; i <= strlen(message); i++) {
    // Set the color based on the character's index (mod number of colors)
    matrix.setTextColor(colors[i % 8]);
 
    // Print each letter at its corresponding position
    matrix.setCursor(pos, 0);
    matrix.print(message[i]);
 
    // Move the cursor position to the left by 6 pixels (character width)
    pos += 6;
  }
 
  matrix.show();                   // Refresh the display
  delay(100);                      // Delay for scrolling speed
 
  // Move the text left
  textPos--;
 
  // Reset the position when the text has fully scrolled off the left
  int16_t textWidth = 5 * strlen(message); // 6 pixels per character for size 1
  if (textPos < -textWidth) {
    textPos = MATRIX_WIDTH;        // Reset position to scroll again
  }
}