#include <Arduino.h>
#include "Joystick.h"

struct Switch {
  int pin;
  volatile bool state;
  int buttonIndex;
  volatile unsigned long lastDebounceTime;  // Last time the interrupt was triggered
};

// Initialize switches with their respective pin numbers
Switch upShiftButton = {2, false, 0, 0};
Switch downShiftButton = {3, false, 1, 0};

Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  2, 0,                  // Button Count, Hat Switch Count
  false, false, false,   // X, Y, Z
  false, false, false,   // Rx, Ry, Rz
  false, false           // Slider, Dial
);

const unsigned long debounceDelay = 30;

void ISR_upShift() {
  unsigned long currentTime = millis();
  if (currentTime - upShiftButton.lastDebounceTime > debounceDelay) {
    upShiftButton.state = !upShiftButton.state;
    Joystick.setButton(upShiftButton.buttonIndex, upShiftButton.state);
    upShiftButton.lastDebounceTime = currentTime;
  }
}

void ISR_downShift() {
  unsigned long currentTime = millis();
  if (currentTime - downShiftButton.lastDebounceTime > debounceDelay) {
    downShiftButton.state = !downShiftButton.state;
    Joystick.setButton(downShiftButton.buttonIndex, downShiftButton.state);
    downShiftButton.lastDebounceTime = currentTime;
  }
}

void setup() {
  Serial.begin(9600);

  // Set button pins as inputs with internal pull-up resistors
  pinMode(upShiftButton.pin, INPUT);
  pinMode(downShiftButton.pin, INPUT);

  // Attach interrupts to the buttons' pins, triggering on CHANGE
  attachInterrupt(digitalPinToInterrupt(upShiftButton.pin), ISR_upShift, CHANGE);
  attachInterrupt(digitalPinToInterrupt(downShiftButton.pin), ISR_downShift, CHANGE);

  // Initialize joystick
  Joystick.begin();
}

void loop() {
  // Main loop can be left empty or used for low-priority tasks
}
