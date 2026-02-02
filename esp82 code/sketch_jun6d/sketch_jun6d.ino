#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define FLOW_SENSOR_PIN D5  // GPIO14

volatile int flowPulseCount = 0;
unsigned long lastMillis = 0;
float flowRate = 0.0;
float totalLiters = 0.0;

// Create LCD: If your scanner showed 0x27, keep this. If 0x3F, change it.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Interrupt service routine for flow sensor
void IRAM_ATTR flowISR() {
  flowPulseCount++;
}

void setup() {
  Serial.begin(115200);

  // I2C LCD setup on NodeMCU (D1=SDA, D2=SCL)
  Wire.begin(D1, D2);      
  lcd.begin(16, 2);        
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Water Monitor");

  // Flow sensor setup
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowISR, RISING);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= 1000) {  // every 1 second
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));

    // Calculate flow rate in L/min
    flowRate = (flowPulseCount / 7.5);  // For YF-S201: 7.5 pulses = 1 L/min
    float litersPerSecond = flowRate / 60.0;
    totalLiters += litersPerSecond;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Flow: ");
    lcd.print(flowRate, 1);
    lcd.print(" L/m");

    lcd.setCursor(0, 1);
    lcd.print("Total: ");
    lcd.print(totalLiters, 2);
    lcd.print(" L");

    Serial.print("Flow: ");
    Serial.print(flowRate);
    Serial.print(" L/m, Total: ");
    Serial.println(totalLiters);

    flowPulseCount = 0;
    lastMillis = currentMillis;

    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowISR, RISING);
  }
}
