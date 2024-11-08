#include <Arduino.h>
#include <HCSR04.H>

uint16_t getBlinkFrequency(int distance);
void updateLED();

const uint8_t PIN_LED = 5;
const uint8_t PIN_TRIGGER = 15;
const uint8_t PIN_ECHO = 4;
const uint8_t DISTANCE_MULTIPLIER = 20;
const unsigned long SENSOR_READ_INTERVAL = 100;

const uint8_t RANGE_SIZE = 3;
const uint8_t NUM_RANGES = 10;
const uint8_t MAX_DISTANCE = RANGE_SIZE * NUM_RANGES;

const uint16_t BLINK_FREQUENCIES[NUM_RANGES] = {
    50,
    100,
    200,
    300,
    400,
    500,
    750,
    1000,
    1500,
    2000
};


UltraSonicDistanceSensor distanceSensor(15, 4);
unsigned long lastSensorRead = 0;
unsigned long lastBlink = 0;
uint16_t currentBlinkInterval = 0;
bool ledState = false;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
}


uint16_t getBlinkFrequency(int distance) {
    if (distance < 0 || distance > MAX_DISTANCE) { return 0; }

    uint8_t rangeIndex = distance / RANGE_SIZE;
    
    if (rangeIndex >= NUM_RANGES) {
        return rangeIndex = NUM_RANGES - 1;
    }
    
    return BLINK_FREQUENCIES[rangeIndex];
}

void updateLED() {
    if (currentBlinkInterval == 0) { 
        digitalWrite(PIN_LED, LOW);
        return;
    }

    unsigned long currentMillis = millis();
    
    if (currentMillis - lastBlink >= currentBlinkInterval) {
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
        lastBlink = currentMillis;
    }
}
 
void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        int distance = distanceSensor.measureDistanceCm();
        currentBlinkInterval = getBlinkFrequency(distance);

        lastSensorRead = currentMillis;
    }

    updateLED();
}