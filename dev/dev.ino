#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  
#define SCREEN_ADDRESS 0x3C

#define HALL_SENSOR_PIN D5
#define BUTTON_DISPLAY D0
#define BUTTON_LED D6
#define LED_PIN D7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long previousMillis = 0;
unsigned long lastPulseTime = 0;
int pulseCount = 0;
float speed = 0;
int fuelLevel = 100;
bool showSpeedometer = true;
bool lastState = HIGH; // To detect edges


#define TRIG_PIN D4
#define ECHO_PIN D3


void drawSpeedometer(float speed);
void drawFuelTank(int fuelLevel);

void setup() {
  Serial.begin(9600);
  Serial.println("Started");

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
}

void loop() {
  unsigned long currentMillis = millis();

  // Check hall sensor state
  bool currentState = digitalRead(HALL_SENSOR_PIN);
  if (lastState == HIGH && currentState == LOW) { // Detect falling edge
    pulseCount++;
    unsigned long pulseInterval = currentMillis - lastPulseTime;
    lastPulseTime = currentMillis;

    // Speed calculation (Assume one pulse per wheel revolution)
    float wheelCircumference = 2.1; // Example: 2.1 meters (adjust based on your setup)
    speed = (wheelCircumference / (pulseInterval / 1000.0)) * 3.6; // Convert m/s to km/h
  }
  lastState = currentState; // Store current state for next loop

  // Toggle display mode with button press (D0)
  if (digitalRead(BUTTON_DISPLAY) == LOW) {
    drawSpeedometer(speed);
  } else {

  long duration;
  float distance;

  // Send a 10µs pulse to trigger the sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the time duration for echo
  duration = pulseIn(ECHO_PIN, HIGH);

  // Convert duration to distance (in cm)
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
    drawFuelTank(distance);
  }

  // Button-controlled LED (D6 input -> D7 output)
  int buttonState = digitalRead(BUTTON_LED);
  if(buttonState == 0){
    if(speed < 30){
      analogWrite(LED_PIN,50);
    } 
    else if(speed < 80){
      analogWrite(LED_PIN,400);
    }
    else if(speed > 80){
      analogWrite(LED_PIN,1023);
    }
  }
  else{
  analogWrite(LED_PIN,0);
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
