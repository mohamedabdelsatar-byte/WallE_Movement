#define REMOTEXY_MODE__SOFTSERIAL
#include <SoftwareSerial.h>
#include <RemoteXY.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Initialize PCA9685 (Default I2C address is 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// PCA9685 Pulse constants
#define SERVOMIN  150 // Minimum pulse length count (0 degrees)
#define SERVOMAX  600 // Maximum pulse length count (180 degrees)
#define SERVO_FREQ 50 // Analog servos run at 50Hz

// Define PCA9685 Channels (0-15) instead of Arduino Pins
#define CH_LEFT_ARM   0
#define CH_LEFT_HAND  1
#define CH_RIGHT_ARM  2
#define CH_RIGHT_HAND 3
#define CH_NECK       4

// RemoteXY connection settings 
#define REMOTEXY_SERIAL_RX 10
#define REMOTEXY_SERIAL_TX 11
#define REMOTEXY_SERIAL_SPEED 9600

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   
  { 255,6,0,0,0,175,0,19,0,0,0,82,111,98,111,116,32,67,111,110,
  116,114,111,108,108,101,114,0,31,2,106,200,200,84,1,1,9,0,5,22,
  12,60,60,15,18,43,43,0,2,26,31,1,57,81,24,24,92,40,17,17,
  0,2,31,0,10,21,82,24,24,91,12,20,20,48,4,26,31,79,78,0,
  31,79,70,70,0,129,240,155,71,29,20,65,34,7,64,17,77,111,118,101,
  109,101,110,116,0,129,49,90,71,29,151,30,25,8,64,17,83,101,114,118,
  111,115,0,4,78,50,7,86,135,3,19,74,0,162,26,4,79,12,10,176,
  174,3,19,74,0,205,26,129,43,5,71,29,135,6,24,5,64,17,85,80,
  47,68,79,87,78,0,129,51,19,56,12,178,5,11,5,64,17,83,73,68,
  69,0 };
  
struct {
  int8_t MoveJoy1_X; // from -100 to 100
  int8_t MoveJoy1_Y; // from -100 to 100
  uint8_t button_01; // =1 if button pressed, else =0, from 0 to 1
  uint8_t pushSwitch_01; // =1 if state is ON, else =0, from 0 to 1
  int8_t slider_01; // from 0 to 100
  int8_t slider_02; // from 0 to 100
  uint8_t connect_flag; 
} RemoteXY;   
#pragma pack(pop)

//===========================================DRIVERS====================================

const int R_PWM1 = 3;  const int L_PWM1 = 5; 
const int R_PWM2 = 6;  const int L_PWM2 = 9; 
int BASE_SPEED = 160;

//===========================================GRID_MAP====================================

const int Rows = 14;
const int Cols = 37;
int currentX = 33;
int currentY = 13;
int orientation = 0; 
bool isHunting = false;
int Map[Rows][Cols];

//===========================================SERVOS====================================

//LIMITS for the servos
const int ARM_MIN = 55;
const int ARM_MAX = 160;

const int HAND_MIN = 0;
const int HAND_MAX = 90;

const int NECK_MIN = 50;
const int NECK_MAX = 140;

// SAFE STARTING POSITIONS
// Changed from 0 to prevent the servos from violently snapping on startup
int posLA = ARM_MIN; 
int posRA = ARM_MAX; 
int posLH = HAND_MIN;
int posRH = HAND_MAX;
int posN = 90;

// Helper function to map degrees (0-180) to PCA9685 pulse length
void setServoAngle(uint8_t channel, int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}

//===========================================SETUP====================================

void setup() {
  RemoteXY_Init();
  Serial.begin(9600);

  // Initialize Motor Pins
  pinMode(R_PWM1, OUTPUT); pinMode(L_PWM1, OUTPUT);
  pinMode(R_PWM2, OUTPUT); pinMode(L_PWM2, OUTPUT);

  // Initialize PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  // Set servos to original starting positions safely
  setServoAngle(CH_LEFT_ARM, posLA);
  setServoAngle(CH_LEFT_HAND, posLH);
  setServoAngle(CH_RIGHT_ARM, posRA);
  setServoAngle(CH_RIGHT_HAND, posRH); 
  setServoAngle(CH_NECK, posN);
}

//===========================================LOOP====================================

void loop() {
  RemoteXY_Handler(); // Handle communication with the app

  // RemoteXY joysticks range from -100 to 100
  int rawX = RemoteXY.MoveJoy1_X;
  int rawY = RemoteXY.MoveJoy1_Y;

  // Mapping from (-100, 100) to (-255, 255) for PWM
  int speed = map(rawY, -100, 100, -255, 255);
  int turn = map(rawX, -100, 100, -180, 180);

  if (abs(rawY) > 5 || abs(rawX) > 5) {
    advancedSteering(speed, turn);
    updateNeck(speed, turn);
  } else {
    stopBrake();
  }

  updateServos();
}

/*========================================CONTROL====================================*/

void updateServos(){
  
  // Slider 1 (0 to 100) controls BOTH arms vertically. 
  // If moving the slider up makes the arms go down, swap the words ARM_MIN and ARM_MAX below.
  int targetLA = map(RemoteXY.slider_01, 0, 100, ARM_MIN, ARM_MAX);
  int targetRA = map(RemoteXY.slider_01, 0, 100, ARM_MAX, ARM_MIN);

  // Slider 2 (0 to 100) controls BOTH hands sideways (trash collection).
  int targetLH = map(RemoteXY.slider_02, 0, 100, HAND_MIN, HAND_MAX);
  int targetRH = map(RemoteXY.slider_02, 0, 100, HAND_MAX, HAND_MIN);

  targetLA = constrain(targetLA, ARM_MIN, ARM_MAX);
  targetRA = constrain(targetRA, ARM_MIN, ARM_MAX);

  targetLH = constrain(targetLH, HAND_MIN, HAND_MAX);
  targetRH = constrain(targetRH, HAND_MIN, HAND_MAX);

  setServoAngle(CH_LEFT_ARM, targetLA);
  setServoAngle(CH_RIGHT_ARM, targetRA);
  setServoAngle(CH_LEFT_HAND, targetLH);
  setServoAngle(CH_RIGHT_HAND, targetRH);
}

void updateNeck(int speed, int turn){

  // if the robot doesn't move, the head will look forward
  if(abs(speed) < 10){
    setServoAngle(CH_NECK, 90);
    posN = 90;
    return;
  }

  // if the robot turns right or left
  if(abs(turn) > 15){

    int target = map(turn, -180, 180, NECK_MIN, NECK_MAX);
    target = constrain(target, NECK_MIN, NECK_MAX);

    // smooth movement
    posN = posN + (target - posN) * 0.1;

    setServoAngle(CH_NECK, posN);
  }
  else{
    // if the robot is moving forward, the head goes back to normal
    posN = posN + (90 - posN) * 0.1;
    setServoAngle(CH_NECK, posN);
  }
}

void advancedSteering(int baseSpeed, int turnFactor) {
  
  int leftSpeed = baseSpeed + turnFactor;
  int rightSpeed = baseSpeed - turnFactor;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Drive Left Wheels (Motor 1)
  if (leftSpeed > 0) {
    analogWrite(R_PWM1, leftSpeed);
    analogWrite(L_PWM1, 0);
  } else {
    analogWrite(R_PWM1, 0);
    analogWrite(L_PWM1, abs(leftSpeed));
  }

  // Drive Right Wheels (Motor 2)
  if (rightSpeed > 0) {
    analogWrite(R_PWM2, rightSpeed);
    analogWrite(L_PWM2, 0);
  } else {
    analogWrite(R_PWM2, 0);
    analogWrite(L_PWM2, abs(rightSpeed));
  }
}

/*===========================================HUNT====================================*/

void timedForwardMovement(int time){
  Forward(BASE_SPEED);
  delay(time);
  stopBrake();
}

void timedBackwardsMovement(int time){
  Backward(BASE_SPEED);
  delay(time);
  stopBrake();
}

void timedTurnLeft(int time){
  turnLeft(BASE_SPEED);
  delay(time);
  orientation = (orientation + 3) % 4;
  stopBrake();
}

void timedTurnRight(int time){
  turnRight(BASE_SPEED);
  delay(time);
  orientation = (orientation + 1) % 4;
  stopBrake();
}

/*========================================CRUISE====================================*/

void Forward(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2
  
  analogWrite(R_PWM1, safeSpeed); analogWrite(R_PWM2, safeSpeed);
  analogWrite(L_PWM1, 0); analogWrite(L_PWM2, 0);

}
void Backward(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, safeSpeed); analogWrite(L_PWM2, safeSpeed);
  analogWrite(R_PWM1, 0); analogWrite(R_PWM2, 0);

}

void turnLeft(int speed){
  int safeSpeed = constrain(speed, 0, 255);
   //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, 0); analogWrite(R_PWM1, safeSpeed);
  analogWrite(L_PWM2, safeSpeed); analogWrite(R_PWM2, 0);

}

void turnRight(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, safeSpeed); analogWrite(R_PWM1, 0);
  analogWrite(L_PWM2, 0); analogWrite(R_PWM2, safeSpeed);
}


void stopBrake(){
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, 0); analogWrite(L_PWM2, 0);
  analogWrite(R_PWM1, 0); analogWrite(R_PWM2, 0);
}

void smoothBrakes(){ 
  for(int current_Speed=BASE_SPEED ; current_Speed >= 0 ; current_Speed-=5){
    Forward(current_Speed);
    delay(20);
  }
  stopBrake();

}
/*===========================================CRUISE====================================*/