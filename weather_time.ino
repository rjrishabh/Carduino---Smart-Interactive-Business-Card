#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <time.h>

// --- MATRIX CONFIGURATION ---
#define MATRIX_PIN      0
#define MATRIX_WIDTH    17
#define MATRIX_HEIGHT   9
#define MATRIX_OPTIONS  (NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS)

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_PIN,
  MATRIX_OPTIONS, NEO_GRB + NEO_KHZ800
);

// --- WIFI & API CONFIG ---
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "WIFI PASSWORD";
const char* apiKey   = "OPENWETHER API KEY";
const char* city     = "YOUR CITY";

// --- BUTTON CONFIG ---
#define BUTTON_CENTER 18
volatile bool buttonPressed = false;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// --- VARIABLES ---
String weatherText = "Fetching...";
bool showWeather = false;
bool colonVisible = true;
unsigned long lastBlink = 0;
unsigned long lastWeatherFetch = 0;
const unsigned long fetchInterval = 10 * 60 * 1000; // 10 minutes

// --- ISR ---
void IRAM_ATTR handleButtonInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  buttonPressed = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  Serial.begin(115200);
  matrix.begin();
  matrix.setBrightness(3);
  matrix.setTextWrap(false);
  matrix.setTextColor(matrix.Color(0, 255, 0));
  matrix.fillScreen(0);
  matrix.show();

  pinMode(BUTTON_CENTER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_CENTER), handleButtonInterrupt, FALLING);

  scrollText("Connecting WiFi...", matrix.Color(255, 255, 0));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  scrollText("Syncing Time...", matrix.Color(0, 255, 100));
  configTime(19800, 0, "pool.ntp.org");  // GMT+5:30 for India

  fetchWeatherAndAQI();
  lastWeatherFetch = millis();
}

void loop() {
  if (buttonPressed) {
    portENTER_CRITICAL(&mux);
    buttonPressed = false;
    portEXIT_CRITICAL(&mux);

    showWeather = !showWeather;

    if (showWeather) {
      fetchWeatherAndAQI();  // Optional: fetch only when toggled ON
    }
  }

  if (millis() - lastWeatherFetch > fetchInterval) {
    fetchWeatherAndAQI();
    lastWeatherFetch = millis();
  }

  if (showWeather) {
    scrollText(weatherText, matrix.Color(0, 200, 255));
  } else {
    scrollTime();
  }
}


// --- FUNCTION: Fetch Weather & AQI ---
void fetchWeatherAndAQI() {
  scrollText("Fetching Weather...", matrix.Color(255, 255, 0));

  HTTPClient http;
  String weatherURL = String("http://api.openweathermap.org/data/2.5/weather?q=")
                    + city + "&appid=" + apiKey + "&units=metric";

  http.begin(weatherURL);
  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    weatherText = "Weather Error";
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  float temp = doc["main"]["temp"];
  int humidity = doc["main"]["humidity"];
  const char* desc = doc["weather"][0]["main"];
  float lat = doc["coord"]["lat"];
  float lon = doc["coord"]["lon"];

  // AQI FETCH
  String aqiURL = String("http://api.openweathermap.org/data/2.5/air_pollution?lat=")
                + String(lat, 6) + "&lon=" + String(lon, 6) + "&appid=" + apiKey;

  http.begin(aqiURL);
  code = http.GET();
  if (code != HTTP_CODE_OK) {
    weatherText = "AQI Error";
    http.end();
    return;
  }

  String aqiPayload = http.getString();
  http.end();

  DynamicJsonDocument aqiDoc(512);
  deserializeJson(aqiDoc, aqiPayload);

  int aqi = aqiDoc["list"][0]["main"]["aqi"];
  String aqiText = "";

  switch (aqi) {
    case 1: aqiText = "Good"; break;
    case 2: aqiText = "Fair"; break;
    case 3: aqiText = "Moderate"; break;
    case 4: aqiText = "Poor"; break;
    case 5: aqiText = "Hazardous"; break;
    default: aqiText = "Unknown"; break;
  }

  weatherText = "T:" + String(temp, 1) + "C H:" + String(humidity) + "% " +
                String(desc) + " AQI:" + aqiText;
}

// --- FUNCTION: Scroll Text ---
void scrollText(const String &text, uint16_t color) {
  int textWidth = text.length() * 6;
  for (int x = MATRIX_WIDTH; x > -textWidth; x--) {
    matrix.fillScreen(0);
    matrix.setCursor(x, (MATRIX_HEIGHT - 8) / 2);
    matrix.setTextColor(color);
    matrix.print(text);
    matrix.show();
    delay(60);

   // if (buttonPressed) return;  // Exit scroll early on interrupt
  }
}

void drawWiFiIcon(uint8_t level) {
  matrix.fillScreen(0);

  int baseX = 5;
  int baseY = 4;
  uint16_t color = matrix.Color(0, 150, 255);

  if (level >= 1)
    matrix.drawPixel(baseX, baseY, color);
  if (level >= 2) {
    matrix.drawPixel(baseX - 1, baseY + 1, color);
    matrix.drawPixel(baseX + 1, baseY + 1, color);
  }
  if (level >= 3) {
    matrix.drawPixel(baseX - 2, baseY + 2, color);
    matrix.drawPixel(baseX, baseY + 2, color);
    matrix.drawPixel(baseX + 2, baseY + 2, color);
  }

  matrix.setCursor(10, 1);
  matrix.setTextColor(matrix.Color(255, 255, 0));
  matrix.print("WiFi");

  matrix.show();
}

// --- FUNCTION: Scroll Time ---
void scrollTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  int h = timeinfo.tm_hour;
  int m = timeinfo.tm_min;

  // Always show colon (no blinking)
  String timeStr = String(h / 10) + String(h % 10) + ":" + String(m / 10) + String(m % 10);

  scrollText(timeStr, matrix.Color(0, 255, 0));
}

