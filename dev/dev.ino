#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  
#define SCREEN_ADDRESS 0x3C

// Pin definitions
#define HALL_SENSOR_PIN D5
#define BUTTON_DISPLAY D0
#define BUTTON_LED D6
#define LED_PIN D7
#define TRIG_PIN D4
#define ECHO_PIN D3

// WiFi credentials
const char* ssid = "AltiusNXT";             // Replace with your WiFi SSID
const char* password = "@ltwifi24ds";       // Replace with your WiFi password

// Display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Speed & distance tracking
unsigned long lastPulseTime = 0;
float speed = 0;
bool lastState = HIGH;

// Timing for ThingSpeak update
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 20000;

void setup() {
  Serial.begin(9600);

  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DISPLAY, INPUT);
  pinMode(BUTTON_LED, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  unsigned long currentMillis = millis();

  // Hall sensor for speed
  bool currentState = digitalRead(HALL_SENSOR_PIN);
  if (lastState == HIGH && currentState == LOW) {
    unsigned long pulseInterval = currentMillis - lastPulseTime;
    lastPulseTime = currentMillis;

    float wheelCircumference = 2.1;  // In meters
    speed = (wheelCircumference / (pulseInterval / 1000.0)) * 3.6; // m/s to km/h
  }
  lastState = currentState;

  // Distance measurement
  long duration;
  float distance;
  long mappedDistance;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;
  mappedDistance = map(distance, 20, 100, 100, 0);

  // Display content
  if (digitalRead(BUTTON_DISPLAY) == LOW) {
    drawSpeedometer(speed);
  } else {
    drawFuelTank(mappedDistance);

    // Send data to ThingSpeak every 20 seconds
    if (currentMillis - lastSendTime >= sendInterval) {
      sendToThingSpeak(mappedDistance);
      lastSendTime = currentMillis;
    }
  }

  // LED control
  int buttonState = digitalRead(BUTTON_LED);
  if (buttonState == LOW) {
    if (speed < 30) analogWrite(LED_PIN, 50);
    else if (speed < 80) analogWrite(LED_PIN, 400);
    else analogWrite(LED_PIN, 1023);
  } else {
    analogWrite(LED_PIN, 0);
  }
}

void drawSpeedometer(float speed) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int centerX = 64, centerY = 45, radius = 25;

  for (int angle = 180; angle >= 0; angle -= 15) {
    float rad = radians(angle);
    int x1 = centerX + radius * cos(rad);
    int y1 = centerY - radius * sin(rad);
    int x2 = centerX + (radius - 5) * cos(rad);
    int y2 = centerY - (radius - 5) * sin(rad);
    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  }

  float needleAngle = map(speed, 0, 140, 180, 0);
  float radNeedle = radians(needleAngle);
  int needleX = centerX + (radius - 10) * cos(radNeedle);
  int needleY = centerY - (radius - 10) * sin(radNeedle);
  display.drawLine(centerX, centerY, needleX, needleY, SSD1306_WHITE);

  display.setCursor(50, 0);
  display.setTextSize(2);
  display.print(speed);
  display.setCursor(40, 50);
  display.print("km/h");

  display.display();
}

void drawFuelTank(int fuelLevel) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 0);
  display.print("Fuel Tank");

  display.drawRoundRect(40, 15, 30, 45, 10, SSD1306_WHITE);
  display.drawLine(50, 10, 60, 10, SSD1306_WHITE);

  int fuelHeight = map(fuelLevel, 0, 100, 0, 40);
  display.fillRoundRect(41, 55 - fuelHeight, 28, fuelHeight, 10, SSD1306_WHITE);

  display.setCursor(80, 30);
  display.setTextSize(2);
  display.print(fuelLevel);
  display.print("%");

  display.display();
}

void sendToThingSpeak(int distance) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://api.thingspeak.com/update?api_key=MF4S9JJ3OT534KF5&field2=" + String(distance);

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.print("ThingSpeak Response: ");
      Serial.println(httpCode);
    } else {
      Serial.println("Failed to send data");
    }

    http.end();
  }
}
