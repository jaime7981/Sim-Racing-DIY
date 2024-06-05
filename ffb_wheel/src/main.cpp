#include <Arduino.h>

#define NODEBUG
#define COMINO
#ifdef _VARIANT_ARDUINO_DUE_X_
#define Serial SerialUSB
#endif

// the digits mean Mmmmrrr (M=Major,m=minor,r=revision)
#define SKETCH_VERSION 3000001

#include "Joystick.h"
#include "DigitalWriteFast.h"
#include "config.h"
#include "order.h"

// -------------------------
// Various global variables
// -------------------------
unsigned long lastEffectsUpdate;
unsigned long nextJoystickMillis;
unsigned long nextEffectsMillis;

// --------------------------
// Joystick related variables
// --------------------------
#define maxY 254

// Define Pins
#define ROLL_EN 8
#define ROLL_R_PWM 10
#define ROLL_L_PWM 9

// Encoder
#define encoderPinA 3
#define encoderPinB 2

#define ENCODER_MAX_VALUE 5040
#define ENCODER_MIN_VALUE -5040
#define WHEEL_MAX_DEGREE 360
#define WHEEL_MIN_DEGREE -360

#define MOTOR_MIN_ACTIVATION 80
#define MOTOR_MAX_ACTIVATION 244

int32_t  currentPosition = 0;
volatile int8_t oldState = 0;
const int8_t KNOBDIR[] = {
  0, 1, -1, 0,
  -1, 0, 0, 1,
  1, 0, 0, -1,
  0, -1, 1, 0
};

bool isOutOfRange = false;
volatile long value = 0;

bool is_connected = false;
bool forces_requested = false;
bool pos_updated = false;

int lastX;
int lastY;
int lastVelX;
int lastVelY;
int lastAccelX;
int lastAccelY;

EffectParams effects[2];
int16_t forces[2] = {0, 0};

Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  0, 0, // Button Count, Hat Switch Count
  true, false, false, // X, Y, Z
  false, false, false, // Rx, Ry, Rz
  false, false // slider, dial
);

void tick(void)
{
  int sig1 = digitalReadFast(encoderPinA);
  int sig2 = digitalReadFast(encoderPinB);
  int8_t thisState = sig1 | (sig2 << 1);

  if (oldState != thisState) {
    currentPosition += KNOBDIR[thisState | (oldState<<2)];
    oldState = thisState;
  } 
}

void setupMotorFFB()
{
  pinMode(ROLL_EN, OUTPUT);
  pinMode(ROLL_R_PWM, OUTPUT);
  pinMode(ROLL_L_PWM, OUTPUT);
}

void setupJoystick() {
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPinA),tick,CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB),tick,CHANGE); 
  Joystick.setXAxisRange(WHEEL_MIN_DEGREE, WHEEL_MAX_DEGREE);

  Gains gains[FFB_AXIS_COUNT];
  gains[0].frictionGain = friction_gain;
  gains[1].frictionGain = friction_gain;
  Joystick.setGains(gains);
  Joystick.begin();
}

void DriveMotors() {
  if (forces[0] == 0) {
    digitalWrite(ROLL_EN, LOW);
  }
  else {
    digitalWrite(ROLL_EN, HIGH);
  }

  if(!isOutOfRange){
    if(forces[0] > 0) {
      analogWrite(ROLL_R_PWM,map(abs(forces[0]), 0, 10000, MOTOR_MIN_ACTIVATION, MOTOR_MAX_ACTIVATION));
    } else {
      analogWrite(ROLL_L_PWM,map(abs(forces[0]), 0, 10000, MOTOR_MIN_ACTIVATION, MOTOR_MAX_ACTIVATION));
    }
  }else{
    if(value < 0){
      analogWrite(ROLL_L_PWM,map(abs(forces[0]), 0, 10000, MOTOR_MIN_ACTIVATION, MOTOR_MAX_ACTIVATION));
    }else{
      analogWrite(ROLL_R_PWM,map(abs(forces[0]), 0, 10000, MOTOR_MIN_ACTIVATION, MOTOR_MAX_ACTIVATION));
    }
  }
}

void write_order(enum Order myOrder)
{
	uint8_t* Order = (uint8_t*) &myOrder;
  Serial.write(Order, sizeof(uint8_t));
}

void updateEffects(bool recalculate){
  for (int i =0; i < 2; i++) {
    effects[i].frictionMaxPositionChange = frictionMaxPositionChangeCfg;
    effects[i].inertiaMaxAcceleration = inertiaMaxAccelerationCfg;
    effects[i].damperMaxVelocity = damperMaxVelocityCfg;
  }

  /*
  effects[0].springMaxPosition = maxX;
  effects[0].springPosition = pos[0];
  effects[1].springMaxPosition = maxY;
  effects[1].springPosition = pos[1];
  */

  // New Effects params
  effects[0].springMaxPosition = ENCODER_MAX_VALUE;
  effects[0].springPosition = value;
  effects[1].springMaxPosition = maxY;
  effects[1].springPosition = 0;

  unsigned long currentMillis;
  currentMillis = millis();
  int16_t diffTime = currentMillis - lastEffectsUpdate;

  if (diffTime > 0 && recalculate) {
    lastEffectsUpdate = currentMillis;
    int16_t positionChangeX = value - lastX;
    int16_t positionChangeY = 0 - lastY;
    int16_t velX = positionChangeX / diffTime;
    int16_t velY = positionChangeY / diffTime;
    int16_t accelX = ((velX - lastVelX) * 10) / diffTime;
    int16_t accelY = ((velY - lastVelY) * 10) / diffTime;

    effects[0].frictionPositionChange = velX;
    effects[1].frictionPositionChange = velY;
    effects[0].inertiaAcceleration = accelX;
    effects[1].inertiaAcceleration = accelY;
    effects[0].damperVelocity = velX;
    effects[1].damperVelocity = velY;

    #ifdef DEBUG
    write_order(LOG);
    Serial.print("t:");
    Serial.print(currentMillis);
    Serial.print(",x:");
    // Serial.print(pos[0]);
    Serial.print(value);
    // Serial.print(",y:");
    // Serial.print(pos[1]);
    /* Serial.print(",Cx:"); */
    /* Serial.print(positionChangeX); */
    // Serial.print(",Cy:");
    // Serial.print(positionChangeY);
    Serial.print(",Vx:");
    Serial.print(velX);
    // Serial.print(",Vy:");
    // Serial.print(velY);
    // Serial.print(",Ax:");
    // Serial.print(accelX);
    // Serial.print(",Ay:");
    // Serial.print(accelY);
    #endif

    lastX = value;
    lastY = 0;
    lastVelX = velX;
    lastVelY = velY;
    lastAccelX = accelX;
    lastAccelY = accelY;
  } else {
    effects[0].frictionPositionChange = lastVelX;
    effects[1].frictionPositionChange = lastVelY;
    effects[0].inertiaAcceleration = lastAccelX;
    effects[1].inertiaAcceleration = lastAccelY;
    effects[0].damperVelocity = lastVelX;
    effects[1].damperVelocity = lastVelY;
  }

  Joystick.setEffectParams(effects);
  Joystick.getForce(forces);

  #ifdef DEBUG
  if (diffTime > 0 && recalculate) {
    Serial.print(",Fx:");
    Serial.print(forces[0]);
    Serial.print(",Fy:");
    Serial.println(forces[1]);
  }
  #endif
}

#ifdef COMINO
Order read_order()
{
	return (Order) Serial.read();
}

void wait_for_bytes(int num_bytes, unsigned long timeout)
{
	unsigned long startTime = millis();
	//Wait for incoming bytes or exit if timeout
	while ((Serial.available() < num_bytes) && (millis() - startTime < timeout)){}
}

// NOTE : Serial.readBytes is SLOW
// this one is much faster, but has no timeout
void read_signed_bytes(int8_t* buffer, size_t n)
{
	size_t i = 0;
	int c;
	while (i < n)
	{
		c = Serial.read();
		if (c < 0) break;
		*buffer++ = (int8_t) c; // buffer[i] = (int8_t)c;
		i++;
	}
}

int8_t read_i8()
{
	wait_for_bytes(1, 100); // Wait for 1 byte with a timeout of 100 ms
  return (int8_t) Serial.read();
}

int16_t read_i16()
{
  int8_t buffer[2];
	wait_for_bytes(2, 100); // Wait for 2 bytes with a timeout of 100 ms
	read_signed_bytes(buffer, 2);
  return (((int16_t) buffer[0]) & 0xff) | (((int16_t) buffer[1]) << 8 & 0xff00);
}

int32_t read_i32()
{
  int8_t buffer[4];
	wait_for_bytes(4, 200); // Wait for 4 bytes with a timeout of 200 ms
	read_signed_bytes(buffer, 4);
  return (((int32_t) buffer[0]) & 0xff) | (((int32_t) buffer[1]) << 8 & 0xff00) | (((int32_t) buffer[2]) << 16 & 0xff0000) | (((int32_t) buffer[3]) << 24 & 0xff000000);
}

void write_i8(int8_t num)
{
  Serial.write(num);
}

void write_i16(int16_t num)
{
	int8_t buffer[2] = {(int8_t) (num & 0xff), (int8_t) (num >> 8)};
  Serial.write((uint8_t*)&buffer, 2*sizeof(int8_t));
}

void write_i32(int32_t num)
{
	int8_t buffer[4] = {(int8_t) (num & 0xff), (int8_t) (num >> 8 & 0xff), (int8_t) (num >> 16 & 0xff), (int8_t) (num >> 24 & 0xff)};
  Serial.write((uint8_t*)&buffer, 4*sizeof(int8_t));
}

void sendForces() {
  write_order(FORCES);
  write_i32(forces[0]);
  write_i32(forces[1]);
}

void get_messages_from_serial()
{
  if(Serial.available() > 0)
  {
    // The first byte received is the instruction
    Order order_received = read_order();

    if(order_received == HELLO)
    {
      // If the cards haven't say hello, check the connection
      if(!is_connected)
      {
        is_connected = true;
        write_order(HELLO);
      }
      else
      {
        // If we are already connected do not send "hello" to avoid infinite loop
        write_order(ALREADY_CONNECTED);
      }
    }
    else if(order_received == ALREADY_CONNECTED)
    {
      is_connected = true;
    }
    else
    {
      switch(order_received)
      {
        case POSITION:
        {
          int16_t x = read_i16();
          // int16_t y = read_i16();
          value = x;
          // pos[1] = y;
          pos_updated = true;
          break;
        }
        case FORCES:
        {
          forces_requested = true;
          break;
        }
        case VERSION:
        {
          write_order(VERSION);
          write_i32(SKETCH_VERSION);
          break;
        }
        case CONFIG:
        {
          float defaultSpringGain = read_i8() / 100.0;

          Gains *gains = Joystick.getGains();
          gains[0].defaultSpringGain = defaultSpringGain;
          gains[1].defaultSpringGain = defaultSpringGain;
          break;
        }
        // Unknown order
        default:
          write_order(ERROR);
          write_i16(404);
          return;
      }
    }
    write_order(RECEIVED); // Confirm the reception
  }
}

#endif

int encoderTicksToDegrees(int encoderTicks) {
    return map(encoderTicks, ENCODER_MIN_VALUE, ENCODER_MAX_VALUE, WHEEL_MIN_DEGREE, WHEEL_MAX_DEGREE);
}

void readEncoder() {
  value = currentPosition;
  
  if(value > ENCODER_MAX_VALUE) {
    isOutOfRange = true;
    value = ENCODER_MAX_VALUE;
  } else if(value < ENCODER_MIN_VALUE) {
    isOutOfRange = true;
    value = ENCODER_MIN_VALUE;
  } else {
    isOutOfRange = false;
  }

  int wheelDegrees = encoderTicksToDegrees(value);
  Joystick.setXAxis(wheelDegrees);
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  setupMotorFFB();
  setupJoystick();

  lastEffectsUpdate = 0;
  nextJoystickMillis = 0;
  nextEffectsMillis = 0;
}

void loop() {
  #ifdef COMINO
  get_messages_from_serial();
  #endif

  unsigned long currentMillis;
  currentMillis = millis();

  if (currentMillis >= nextJoystickMillis) {
    readEncoder();
    nextJoystickMillis = currentMillis + 2;

    if (currentMillis >= nextEffectsMillis || pos_updated) {
      updateEffects(true);
      nextEffectsMillis = currentMillis + 100;
      pos_updated = false;
    } else {
      updateEffects(false);
    }

    #ifdef COMINO
    if (forces_requested) {
      sendForces();
      forces_requested = false;
    }
    #endif
  }

  DriveMotors();
}
