#include <Wire.h>
#include <EEPROM.h>
#include "FastIMU.h"
#include "Madgwick.h"

#define IMU_ADDRESS 0x68
// #define PERFORM_CALIBRATION // Comment to disable startup calibration
MPU9250 IMU;
Madgwick filter;

#define WhiteBtnPin 4
#define YellowBtnPin 3
#define BlueBtnPin 6 
#define ThumbClickBtnPin 5
#define GreenBtnPin 2
#define JoyXPin A1
#define JoyYPin A2

calData calib = {0};
AccelData accelData;
GyroData gyroData;
MagData magData;

int joyX;
int joyY;

unsigned long lastUpdate = 0;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Wire.setClock(400000); // 400kHz clock
  pinMode(WhiteBtnPin, INPUT_PULLUP);
  pinMode(YellowBtnPin, INPUT_PULLUP);
  pinMode(BlueBtnPin, INPUT_PULLUP);
  pinMode(GreenBtnPin, INPUT_PULLUP);
  pinMode(ThumbClickBtnPin, INPUT_PULLUP);

  // Start serial monitor at this baud rate
  Serial.begin(115200);
  while (!Serial){
    ;
  }

  // IMU initialization and calibration?
  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0)
  {
    Serial.print("Error initializing IMU: ");
    Serial.println(err);
    while(true)
    {
      ;
    }
  }

#ifdef PERFORM_CALIBRATION
  Serial.println("Performing IMU Calibration!");
  if (IMU.hasMagnetometer()){
    delay(1000);
    Serial.println("Move IMU in figure 8 pattern until done.");
    delay(3000);
    IMU.calibrateMag(&calib);
    Serial.println("Magnetic Calibration done!");
  }
  else{
    delay(5000);
  }
  delay(5000);

  Serial.println("Keep IMU level.");
  delay(5000);
  IMU.calibrateAccelGyro(&calib);
  Serial.println("Calibration done!");
  Serial.println("Accel biases X/Y/Z: ");
  Serial.print(calib.accelBias[0]);
  Serial.print(", ");
  Serial.print(calib.accelBias[1]);
  Serial.print(", ");
  Serial.println(calib.accelBias[2]);
  Serial.println("Gyro biases X/Y/Z: ");
  Serial.print(calib.gyroBias[0]);
  Serial.print(", ");
  Serial.print(calib.gyroBias[1]);
  Serial.print(", ");
  Serial.println(calib.gyroBias[2]);
  if (IMU.hasMagnetometer()){
    Serial.println("Mag biases X/Y/Z: ");
    Serial.print(calib.magBias[0]);
    Serial.print(", ");
    Serial.print(calib.magBias[1]);
    Serial.print(", ");
    Serial.println(calib.magBias[2]);
    Serial.println("Mag Scale X/Y/Z: ");
    Serial.print(calib.magScale[0]);
    Serial.print(", ");
    Serial.print(calib.magScale[1]);
    Serial.print(", ");
    Serial.println(calib.magScale[2]);
  }
  delay(5000);
  IMU.init(calib, IMU_ADDRESS);
  Serial.println("Writing values to EEPROM!");
  EEPROM.put(210, calib);
  delay(3000);


#endif
  EEPROM.get(210, calib);
  filter.begin(100);
  lastUpdate = millis();
  // printCalibration();
}

void printCalibration(){
  Serial.println("Accel biases X/Y/Z: ");
  Serial.print(calib.accelBias[0]);
  Serial.print(", ");
  Serial.print(calib.accelBias[1]);
  Serial.print(", ");
  Serial.println(calib.accelBias[2]);
  Serial.println("Gyro biases X/Y/Z: ");
  Serial.print(calib.gyroBias[0]);
  Serial.print(", ");
  Serial.print(calib.gyroBias[1]);
  Serial.print(", ");
  Serial.println(calib.gyroBias[2]);
  if (IMU.hasMagnetometer()) {
    Serial.println("Mag biases X/Y/Z: ");
    Serial.print(calib.magBias[0]);
    Serial.print(", ");
    Serial.print(calib.magBias[1]);
    Serial.print(", ");
    Serial.println(calib.magBias[2]);
    Serial.println("Mag Scale X/Y/Z: ");
    Serial.print(calib.magScale[0]);
    Serial.print(", ");
    Serial.print(calib.magScale[1]);
    Serial.print(", ");
    Serial.println(calib.magScale[2]);
  }
  delay(5000);
}

void loop() {
  // put your main code here, to run repeatedly:
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);
  IMU.getMag(&magData);
  
  // Convert gyroscope to radians/sec
  float gx = gyroData.gyroX * PI/180.0;
  float gy = gyroData.gyroY * PI/180.0;
  float gz = gyroData.gyroZ * PI/180.0;

  // Update Madgwick Filter
  filter.update(gx, gy, gz, accelData.accelX, accelData.accelY, accelData.accelZ, magData.magX, magData.magY, magData.magZ);

  // Get orientation in quaternions
  float qw = filter.getQuatW();
  float qx = filter.getQuatX();
  float qy = filter.getQuatY();
  float qz = filter.getQuatZ();

  // Write Data as int
  uint8_t qwInt = (abs(qw) * 100);
  if (qw < 0){
    Serial.write(127);
  }
  else{
    Serial.write(128);
  }
  Serial.write(qwInt);
  uint8_t qxInt = (abs(qx) * 100);
  if (qx < 0){
    Serial.write(127);
  }
  else{
    Serial.write(128);
  }
  Serial.write(qxInt);
  uint8_t qyInt = (abs(qy) * 100);
  if (qy < 0){
    Serial.write(127);
  }
  else {
    Serial.write(128);
  }
  Serial.write(qyInt);
  uint8_t qzInt = (abs(qz) * 100);
  if (qz < 0){
    Serial.write(127);
  }
  else{
    Serial.write(128);
  }
  Serial.write(qzInt);
  // Serial.write(255); 
 

  // Read data from joyStick
  joyX = analogRead(JoyXPin);
  joyY = analogRead(JoyYPin);
  // Serial.print("X Axis: ");
  // Serial.print(joyX);
  // Serial.print(" Y Axis: ");
  // Serial.println(joyY);

  // Read state of the buttons
  if(digitalRead(WhiteBtnPin) == LOW){
    delay(50);
    if(digitalRead(WhiteBtnPin) == LOW){
      Serial.write(3);
    }
    // Serial.println("White Button pressed!");
  }
  if(digitalRead(YellowBtnPin) == LOW){
    delay(50);
    if (digitalRead(YellowBtnPin) == LOW){
      Serial.write(2);
    }
    // Serial.println("Yellow Button Pressed!");
  }
  if(digitalRead(BlueBtnPin) == LOW){
    delay(50);
    if (digitalRead(BlueBtnPin) == LOW){
      Serial.write(4);
    }
    // Serial.println("Blue Button Pressed!");
  }
  if(digitalRead(GreenBtnPin) == LOW){
    delay(50);
    if (digitalRead(GreenBtnPin) == LOW){
      Serial.write(1);
    }
    // Serial.println("Green Button Pressed!");
  }
  if(digitalRead(ThumbClickBtnPin) == LOW){
    delay(50);
    if(digitalRead(ThumbClickBtnPin) == LOW){
      Serial.write(5);
    }
  }
  Serial.write(255);
  delay(50);
}


