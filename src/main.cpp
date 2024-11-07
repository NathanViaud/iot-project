#include <Arduino.h>

// put function declarations here:

#define LASER 15
#define LDR 2
#define BUZZER 5

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LASER, OUTPUT);
  pinMode(LDR, INPUT);

  delay(500);
  digitalWrite(LASER, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  int ldr = analogRead(2);
  Serial.println("light sensor: " + String(ldr));

  if (ldr < 250) { tone(BUZZER, 100); } 
  if (ldr >= 250) { noTone(BUZZER); }

  delay(100);
}