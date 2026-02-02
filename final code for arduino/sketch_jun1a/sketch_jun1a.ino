#include <CheapStepper.h>
#include <Servo.h>

#define IR_SENSOR 5
#define PROXI_SENSOR 6
#define RAIN_SENSOR 4
#define BUZZER 12
#define POT_PIN A0

Servo servo1;
CheapStepper stepper(8, 9, 10, 11);

bool triggered = false;

void setup() {
  Serial.begin(9600);

  pinMode(PROXI_SENSOR, INPUT_PULLUP);
  pinMode(IR_SENSOR, INPUT);
  pinMode(RAIN_SENSOR, INPUT);
  pinMode(BUZZER, OUTPUT);

  servo1.attach(7);
  stepper.setRpm(17);
  servo1.write(70);  // initial position

  Serial.println("System Ready");
}

void activateSystem() {
  Serial.println("Sensor triggered -> activating servo & stepper");

  tone(BUZZER, 1000, 200);
  stepper.moveDegreesCW(180);  // stepper rotates 180°
  delay(1000);

  servo1.write(0);     // rotate to 0°
  delay(800);
  servo1.write(180);   // rotate to 180°
  delay(800);
  servo1.write(70);    // return to initial
  delay(800);

  stepper.moveDegreesCCW(180);  // stepper returns
  delay(500);

  noTone(BUZZER);
}

void loop() {
  int proxiState = digitalRead(PROXI_SENSOR);
  int rainState = digitalRead(RAIN_SENSOR);
  int irState = digitalRead(IR_SENSOR);

  if ((proxiState == LOW || rainState == LOW || irState == LOW) && !triggered) {
    triggered = true;
    activateSystem();
  }

  if (proxiState == HIGH && rainState == HIGH && irState == HIGH) {
    triggered = false;  // ready for next trigger
  }

  delay(100);
}
