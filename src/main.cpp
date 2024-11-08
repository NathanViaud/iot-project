#include <Arduino.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "Sweetjuju";
const char* password = "jujulien38";

WebServer server(80);

uint16_t getBlinkFrequency(int distance);
void updateLED();
void handleRoot();
void handleData();

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
int lastDistance = 0;

void setup() {
    pinMode(PIN_LED, OUTPUT);
    Serial.begin(115200);
  
    // Setup Wifi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Setup server routes
    server.on("/", handleRoot);
    server.on("/data", handleData);
    
    server.begin();
    Serial.println("HTTP server started");
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
    server.handleClient();

    unsigned long currentMillis = millis();

    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastDistance = distanceSensor.measureDistanceCm();
        currentBlinkInterval = getBlinkFrequency(lastDistance);

        lastSensorRead = currentMillis;
    }

    updateLED();
}

void handleRoot() {
    String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Distance Sensor Monitor</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body { font-family: Arial; text-align: center; margin: 20px; }
            #distance { font-size: 24px; margin: 20px 0; }
            #blinkFreq { font-size: 18px; }
        </style>
        <script>
            function updateData() {
                fetch('/data')
                    .then(response => response.json())
                    .then(data => {
                        document.getElementById('distance').innerHTML = 
                            'Distance: ' + data.distance + ' cm';
                        document.getElementById('blinkFreq').innerHTML = 
                            'Blink Frequency: ' + data.blinkInterval + ' ms';
                    });
            }
            setInterval(updateData, 500);
        </script>
    </head>
    <body>
        <h1>Distance Sensor Monitor</h1>
        <div id="distance">Loading...</div>
        <div id="blinkFreq"></div>
    </body>
    </html>
    )";
    server.send(200, "text/html", html);
}

void handleData() {
    StaticJsonDocument<200> doc;    
    doc["distance"] = lastDistance;
    doc["blinkInterval"] = currentBlinkInterval;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}