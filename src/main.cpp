#include <Arduino.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// Wifi credientials
const char* ssid = "Pixel_4255";
const char* password = "aaaaaaaa";

// MQTT Broker settings
const char* mqtt_broker = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_distance = "bat-radar/distance";
const char* mqtt_topic_led = "bat-radar/led";

WiFiClient espClient;
PubSubClient client(espClient);

uint16_t getBlinkFrequency(int distance);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void publishSensorData(int distance, unsigned int collisions);
void updateLED();


const uint8_t PIN_LED = 5;
const uint8_t PIN_LED2 = 18;
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
unsigned int collisions = 0;
bool collided = false;

void setup() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
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
    
    // Setup MQTT
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    client.setBufferSize(512);

}


uint16_t getBlinkFrequency(int distance) {
    if (distance < 0 || distance > MAX_DISTANCE) { return 0; }

    uint8_t rangeIndex = distance / RANGE_SIZE;
    
    if (rangeIndex >= NUM_RANGES) {
        return rangeIndex = NUM_RANGES - 1;
    }
    
    return BLINK_FREQUENCIES[rangeIndex];
}
 
void loop() {
    client.loop();

    if (!client.connected()) {
        reconnect();
    }

    unsigned long currentMillis = millis();

    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastDistance = distanceSensor.measureDistanceCm();
        currentBlinkInterval = getBlinkFrequency(lastDistance);
        if (lastDistance <= 1 && lastDistance > 0 && !collided) {
            collisions ++;
            collided = true; 
        } else if (collided && lastDistance > 0) { collided = false; }

        publishSensorData(lastDistance, collisions);
        lastSensorRead = currentMillis;
    }

    updateLED();
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (String(topic) == mqtt_topic_led) {
        // Handle LED control messages if needed
        if (message == "ON") {
            digitalWrite(PIN_LED2, HIGH);
        } else if (message == "OFF") {
            digitalWrite(PIN_LED2, LOW);
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32Client-" + String(WiFi.macAddress());
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            Serial.println(clientId);
            client.subscribe(mqtt_topic_led);
        } else {
            Serial.print("failed, rc= ");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }

    }
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

int count = 0;
void publishSensorData(int distance, unsigned int collisions) {
    if (count >= 10) {
        JsonDocument doc;
        doc["distance"] = distance;
        doc["collisions"] = collisions;
        doc["ledState"] = ledState;
        
        char buffer[200];
        serializeJson(doc, buffer);
        Serial.println("Sending data");
        Serial.println(client.connected());
        client.publish(mqtt_topic_distance, buffer);
        count = 0;
    }

    count ++;
}