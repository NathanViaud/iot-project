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
unsigned int collisions = 0;
bool collided = false;

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
        if (lastDistance <= 1 && lastDistance > 0 && !collided) {
            collisions ++;
            collided = true; 
        } else if (collided && lastDistance > 0) { collided = false; }

        lastSensorRead = currentMillis;
    }

    updateLED();
}

void handleRoot() {
    String html = R"(
    <!DOCTYPE html>
    <html>
        <head>
            <title>Bat radar</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <link rel="stylesheet" href="https://unpkg.com/tailwindcss@2.2.19/dist/tailwind.min.css" />
            <script>
                function updateData() {
                    fetch('/data')
                        .then(response => response.json())
                        .then(data => {
                            document.getElementById('distance').innerHTML = 
                                'Distance: ' + data.distance + ' cm';
                            document.getElementById('collisions').innerHTML = data.collisions;

                            let color = "#14b8a6"
                            if (data.distance < 25) {
                                color = "#ef4444";
                            } else if (data.distance < 50) {
                                color = "#f59e0b";
                            }

                            const distanceBar = document.getElementById('distance-bar');
                            distanceBar.style = 'width: ' + data['distance-bar'] + '%';
                            distanceBar.style.backgroundColor = color;
                        });
                }
                setInterval(updateData, 500);
            </script>
        </head>
        <body class="text-center p-4 flex flex-col gap-5">
            <h1 class="text-4xl font-bold">Bat Radar</h1>
            <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-4">
                <div class="shadow p-4 rounded-lg col-span-1 sm:col-span-2 border border-slate-200">
                    <h2 id="distance" class="text-2xl mb-3">Distance: <span id="distance">0 cm</span></h2>
                    <div class='w-full bg-gray-100 rounded-3xl h-2.5'>
                        <div id="distance-bar" class='h-2.5 rounded-3xl transition-all' style='width: 0%'></div>
                    </div>
                </div> 
                <div class="shadow p-4 rounded-lg border border-slate-200">
                   <h2 class="text-2xl"><span id="collisions">0</span> Collisions</h2> 
                </div>
            </div>
        </body>
    </html>
    )";
    server.send(200, "text/html", html);
}

void handleData() {
    JsonDocument doc;    
    doc["distance"] = lastDistance;
    doc["collisions"] = collisions;
    // the closer the distance is to 0 the higher the progress bar is
    if (lastDistance > 150) {
        doc["distance-bar"] = 0;
    } else {
        doc["distance-bar"] = map(lastDistance, 0, 150, 100, 0);
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}