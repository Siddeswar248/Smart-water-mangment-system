 #include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <math.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Water level input pins
const int levelPins[5] = {2, 3, 4, 5, 6};
bool levelStatus[5] = {false};
const int ledPins[5] = {7, 8, 9, 10, 11};

// Relay pin
const int relayPin = A1;

// DFPlayer Mini
SoftwareSerial mySerial(12, 13); // RX, TX
DFRobotDFPlayerMini dfplayer;

// Temperature sensor
const int tempPin = A0;

// State tracking
int currentLevel = -1;
int lastLevel = -1;
bool pumpOn = false;
unsigned long lastTempPlayTime = 0;
const unsigned long tempPlayInterval = 60000;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found"));
    while (1);
  }
  display.clearDisplay();
  display.display();

  // Pins
  for (int i = 0; i < 5; i++) {
    pinMode(levelPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // DFPlayer
  if (!dfplayer.begin(mySerial)) {
    Serial.println(F("DFPlayer not found"));
  } else {
    dfplayer.volume(25);
  }
}

void loop() {
  // Read level
  for (int i = 0; i < 5; i++) {
    levelStatus[i] = !digitalRead(levelPins[i]);
  }
  if (levelStatus[0]) currentLevel = 1;
  else if (levelStatus[1]) currentLevel = 2;
  else if (levelStatus[2]) currentLevel = 3;
  else if (levelStatus[3]) currentLevel = 4;
  else if (levelStatus[4]) currentLevel = 5;
  else currentLevel = -1;

  // LEDs
  for (int i = 0; i < 5; i++) {
    digitalWrite(ledPins[i], (i == currentLevel - 1) ? HIGH : LOW);
  }

  // Relay logic
  if (currentLevel != -1 && currentLevel > 1) {
    digitalWrite(relayPin, HIGH);
    pumpOn = true;
  } else {
    digitalWrite(relayPin, LOW);
    pumpOn = false;
  }

  // DFPlayer level audio (play only on change)
  if (currentLevel != lastLevel && currentLevel != -1) {
    dfplayer.play(currentLevel); // 1 to 5 mapped to levels
    lastLevel = currentLevel;
  }

  // Temperature
  int raw = analogRead(tempPin);
  float voltage = raw * (5.0 / 1023.0);
  float resistance = (5.0 - voltage) * 10000 / voltage;
  float temperature = 1.0 / (log(resistance / 100000.0) / 3950.0 + 1.0 / 298.15) - 273.15;

  if (temperature >= 45.0 && millis() - lastTempPlayTime > tempPlayInterval) {
    dfplayer.play(6); // High temp alert
    lastTempPlayTime = millis();
  }

  // OLED Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 0);
  display.print("Water Level: ");
  display.println(currentLevel == -1 ? 0 : currentLevel);
  display.setCursor(40, 10);
  display.print("Level ");
  display.print(currentLevel == -1 ? 0 : currentLevel);
  display.println(" Detected");

  display.setCursor(40, 58);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.print(" C");

  // Tank Display
  int tankX = 10, tankY = 8, tankWidth = 40, tankHeight = 50;
  display.drawRoundRect(tankX, tankY, tankWidth, tankHeight, 6, SSD1306_WHITE);
  display.drawRect(tankX + (tankWidth - 12) / 2, tankY - 8, 12, 8, SSD1306_WHITE);

  int fillHeight = 0;
  if (currentLevel != -1)
    fillHeight = (tankHeight / 5) * (6 - currentLevel);

  if (fillHeight > 0) {
    int fillX = tankX + 3;
    int fillY = tankY + tankHeight - fillHeight;
    int fillW = tankWidth - 6;
    int fillAdj = fillHeight - 2;
    if (fillAdj < 0) fillAdj = 0;
    display.fillRoundRect(fillX, fillY, fillW, fillAdj, 4, SSD1306_WHITE);
    display.drawLine(fillX, fillY, fillX + fillW, fillY, SSD1306_BLACK);
  }

  // Percentage text
  int percent = currentLevel == -1 ? 0 : 100 - (currentLevel - 1) * 25;
  String percentText = String(percent) + "%";
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
  int textY = tankY + tankHeight - (tankHeight / 5) * (6 - currentLevel) + ((tankHeight / 5 - h) / 2);
  int textX = tankX + (tankWidth - w) / 2;
  bool insideFill = currentLevel != -1 && (textY + h > tankY + tankHeight - fillHeight);
  display.setTextColor(insideFill ? SSD1306_BLACK : SSD1306_WHITE);
  display.setCursor(textX, textY);
  display.print(percentText);
  display.setTextColor(SSD1306_WHITE);

  // Pump status
  if (pumpOn) {
    display.setTextSize(2);
    display.setCursor(40, 25);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.println("PUMP ON");
  }

  // High temp warning
  if (temperature >= 45.0) {
    display.setTextSize(1);
    display.setCursor(40, 40);
    display.println("!!! TEMP HIGH !!!");
    display.fillTriangle(10, 50, 20, 30, 30, 50, SSD1306_WHITE);
  }

  // Thermometer icon
  int thermoX = 90, thermoY = 20, thermoWidth = 20, thermoHeight = 40;
  display.drawCircle(thermoX + thermoWidth / 2, thermoY + thermoHeight, 8, SSD1306_WHITE);
  display.drawRoundRect(thermoX + thermoWidth/2 - 5, thermoY, 10, thermoHeight, 5, SSD1306_WHITE);

  float tempBar = constrain(temperature, 0, 60);
  fillHeight = map(tempBar, 0, 60, 0, thermoHeight - 4);
  display.fillCircle(thermoX + thermoWidth / 2, thermoY + thermoHeight, 7, SSD1306_WHITE);
  display.fillRoundRect(thermoX + thermoWidth/2 - 4, thermoY + thermoHeight - fillHeight, 8, fillHeight, 4, SSD1306_WHITE);

  // Temp number
  display.setTextSize(2);
  display.setCursor(thermoX - 20, thermoY + thermoHeight + 4);
  display.print(temperature, 1);
  display.print(" C");

  if (temperature >= 45.0) {
    display.setTextSize(1);
    display.setCursor(thermoX - 30, thermoY + thermoHeight + 24);
    display.println("!!! TEMP HIGH !!!");
    display.fillTriangle(thermoX - 10, thermoY + thermoHeight + 18,
                         thermoX, thermoY + thermoHeight + 38,
                         thermoX + 10, thermoY + thermoHeight + 18, SSD1306_WHITE);
  }

  display.display();
  delay(500); // smooth refresh
}
