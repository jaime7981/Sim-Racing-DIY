#include <Arduino.h>
#include "Joystick.h"

// Define pin numbers for each potentiometer and rumble motors
const int throttlePin = A0;
const int brakePin = A1;
const int clutchPin = A2;
const int throttleRumblePin = 5;
const int brakeRumblePin = 6;
const int clutchRumblePin = 7;

// Define structure for a pedal
struct Pedal {
  int pin;
  int minValue;
  int maxValue;
  int rumblePin;
};

// Define pedals
Pedal throttle = {throttlePin, 0, 255, throttleRumblePin};
Pedal brake = {brakePin, 0, 255, brakeRumblePin};
Pedal clutch = {clutchPin, 0, 255, clutchRumblePin};

// Create joystick object
Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  0, 0, // Button Count, Hat Switch Count
  true, true, true, // X, Y, Z
  false, false, false, // Rx, Ry, Rz
  false, false // slider, dial
);

// Function to read pedal value and map it to a range
int readPedalValue(Pedal pedal) {
  int value = analogRead(pedal.pin);
  return map(value, pedal.minValue, pedal.maxValue, 0, 255);
}

// Function to control rumble motor intensity
void controlRumble(Pedal pedal, int intensity) {
  analogWrite(pedal.rumblePin, intensity);
}

void setup() {
  // Initialize joystick library
  Joystick.begin();

  // Set analog pins as inputs
  pinMode(throttlePin, INPUT);
  pinMode(brakePin, INPUT);
  pinMode(clutchPin, INPUT);

  // Set rumble motor pins as outputs
  pinMode(throttleRumblePin, OUTPUT);
  pinMode(brakeRumblePin, OUTPUT);
  pinMode(clutchRumblePin, OUTPUT);

  // Initialize serial communication
  Serial.begin(9600);
}

void loop() {
  // Read values from pedals
  int throttleValue = readPedalValue(throttle);
  int brakeValue = readPedalValue(brake);
  int clutchValue = readPedalValue(clutch);

  // Update joystick axes
  Joystick.setXAxis(throttleValue);
  Joystick.setYAxis(brakeValue);
  Joystick.setZAxis(clutchValue);

  // Add a small delay
  delay(10);
}
