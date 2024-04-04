#include <Arduino.h>
#include "Joystick.h"

// Define struct for a switch
struct Switch {
  int pin;
  bool state;
  int buttonIndex; // Index of the corresponding joystick button
};

// Define switches
Switch upShiftButton = {2, false, 0}; // Example pin and button index for upshift button
Switch downShiftButton = {3, false, 1}; // Example pin and button index for downshift button

// Define joystick object
Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  2, 0, // Button Count, Hat Switch Count
  false, false, false, // X, Y, Z
  false, false, false, // Rx, Ry, Rz
  false, false // slider, dial
);

// Function to read switch state and set joystick button state
void updateSwitchState(Switch &button) {
  bool currentState = digitalRead(button.pin);

  if (currentState != button.state) {
    button.state = currentState;

    if (button.state) {
      Joystick.releaseButton(button.buttonIndex);
    } else {
      Joystick.pressButton(button.buttonIndex);
    }
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Set button pins as inputs with internal pull-up resistors
  pinMode(upShiftButton.pin, INPUT_PULLUP);
  pinMode(downShiftButton.pin, INPUT_PULLUP);

  // Set initial button state
  updateSwitchState(upShiftButton);
  updateSwitchState(downShiftButton);
}

void loop() {
  // Read and set button states
  updateSwitchState(upShiftButton);
  updateSwitchState(downShiftButton);

  // Small delay to prevent bouncing
  delay(50);
}
