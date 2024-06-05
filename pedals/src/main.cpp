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
  false, false, false, // X, Y, Z
  true, true, true, // Rx, Ry, Rz
  false, false // slider, dial
);

// Function to read pedal value and map it to a range
int readPedalValue(Pedal pedal) {
  const int numReadings = 5;
  int readings[numReadings]; // the readings from the analog input
  int index = 0; // the index of the current reading
  int total = 0; // the running total
  int average = 0; // the average

  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  int value = analogRead(pedal.pin);
  total = total - readings[index]; // subtract the last reading
  readings[index] = value; // read from the sensor
  total = total + readings[index]; // add the reading to the total
  index = (index + 1) % numReadings; // advance to the next position in the array
  average = total / numReadings; // calculate the average

  return map(average, pedal.minValue, pedal.maxValue, 0, 255);
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
  Joystick.setRxAxis(throttleValue);
  Joystick.setRyAxis(brakeValue);
  Joystick.setRzAxis(clutchValue);

  // Add a small delay
  delay(10);
}
