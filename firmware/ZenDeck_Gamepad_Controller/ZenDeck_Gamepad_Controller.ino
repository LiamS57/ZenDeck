#include <Wire.h>
#include <EEPROM.h>
#include <XInput.h>

#define PWM47k  3   //  46875 Hz
#define PWM6        OCR4D
#define PWM6_13_MAX OCR4C

//I2C Controller Addresses
byte controllerAddr = 0x01;         //i2c Address of other half of controller
byte buttons = 0xF1;                //Register for buttons
byte xAxis = 0xF2;                  //Register for X Axis
byte yAxis = 0xF3;                  //Register for Y Axis
byte zAxis = 0xF4;                  //Register for Z Axis
byte rumble = 0xF5;                 //Register for rumble
int i2cButtonCount = 6;            //Number of buttons on i2c

//Default Joystick calibration settings and EEPROM storage Address
int minLeftX = 0; //EEPROM Adr = 1
int maxLeftX = 720; //EEPROM Adr = 3
int midLeftX = 360; //EEPROM Adr = 5

int minLeftY = 0; //EEPROM Adr = 7
int maxLeftY = 600; //EEPROM Adr = 9
int midLeftY = 390; //EEPROM Adr = 11

int minRightY = 0; //EEPROM Adr = 13
int maxRightY = 760; //EEPROM Adr = 15
int midRightY = 400; //EEPROM Adr = 17

int minRightX = 0; //EEPROM Adr = 19
int maxRightX = 760; //EEPROM Adr = 21
int midRightX = 390; //EEPROM Adr = 23

int minL2 = 10;  //EEPROM Adr = 25
int maxL2 = 230;  //EEPROM Adr = 27
int minR2 = 10;  //EEPROM Adr = 29
int maxR2 = 230; //EEPROM Adr = 31

//Joystick LookUp Tables, compiled at startup or after joystick calibration.
byte leftXLUT[400];
byte leftYLUT[400];
byte rightXLUT[400];
byte rightYLUT[400];

float scaleFactor;    //-----------------------------
int scaleMax = 255;   //Used for LUT calculations
int lutLength = 400;  //-----------------------------

//Joystick Settings
const bool invertLeftY = true;        //------------------------------------------
const bool invertLeftX = false;       //Change these settings for Inverted mounting 
const bool invertRightY = false;      //of joysticks.
const bool invertRightX = false;      //------------------------------------------
const int deadBand = 25;              //Joystick deadband settings. 
const bool useDeadband = true;        //
int joyDeadZone = 25;                 //Secondary Joystick Deadband. Can't remember why I added this, may be able to replace with deadBandLeft or deadBandRight.
const int earlyStop = 30;             //Distance from end of travel to achieve full axis movement. 
const int triggerDeadband = 25;       //Trigger Deadband

boolean calibrationMode = false;  //Set to true to open Calibration menu
int calibrationStep = 1;          //Stage of calibration process

//Left Controller IO setup
int buttonCount = 9;                   //Number of buttons connected to Pico
byte buttonPins[9] = {8,9,10,11,5,13,4,12,7}; //L3,L1,Up,Right,Down,Left,Start,Select,Menu
bool analogTriggers = true;            //Set to true for analog triggers, false for digital. Digital mode uses triggerDeadband to simulate a button press
byte rumbleLarge = 6;
#define leftZ A2
#define leftX A0
#define leftY A1

uint8_t rumbleLeft = 0;
uint8_t rumbleRight = 0;


//Button state arrays
byte lastButtonState[15]; //Empty State array for buttons last sent state.
byte currentButtonState[15]; //Empty State array for buttons.

void setup() {
  Wire.begin();
  Serial.begin(9600);

  XInput.setAutoSend(false);  // Wait for all controls before sending
  XInput.begin();
  XInput.setReceiveCallback(xInputCallback);

  while(!Serial);

  for(int i = 0; i < buttonCount; i++){ //Set all button pins as input pullup. Change to INPUT if using external resistors.
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(leftZ, INPUT); //Set input for right trigger
  pinMode(leftX, INPUT); //Set input for right joystick X
  pinMode(leftY, INPUT); //Set input for right joystick Y

  pwm613configure(PWM47k);
  pwmSet6(0);
  //pinMode(rumbleLarge, OUTPUT);

  eepromLoad();
  
  buildLUTs();
  XInput.setJoystickRange(0, scaleMax);

}

void loop() {
  buttonRead();
  if(calibrationMode){
    joystickCalibration();
  } else {
    gamepadMode();
  }
  if(lastButtonState[8] == 1 & lastButtonState[1] == 1 & lastButtonState[10] == 1){ //Logo Button, L1, R1
    //digitalWrite(rumbleLarge, HIGH);
    //delay(50);
    //digitalWrite(rumbleLarge, LOW);
    calibrationMode = true;
  }
}
//L3,L1,Up,Right,Down,Left,Start,Select,Menu
void gamepadMode(){ //Controller Gamepad mode
  joystickInput();
  //Buttons
  for(int i = 0; i < buttonCount + i2cButtonCount; i++){
    if(lastButtonState[i] != currentButtonState[i]){
      currentButtonState[i] = lastButtonState[i];
    }
  }
  if(rumbleLeft > 0){
    //analogWrite(rumbleLarge, rumbleLeft);
    PWM6 = rumbleLeft;
  } else {
    PWM6 = 0;
    //digitalWrite(rumbleLarge, 0);
  }
  if(rumbleRight > 0){
    i2cRumble(rumbleRight / 10.24);
  } else {
    i2cRumble(0);
  }

  XInput.setButton(BUTTON_A, lastButtonState[14]);//14
	XInput.setButton(BUTTON_B, lastButtonState[13]);//13
	XInput.setButton(BUTTON_X, lastButtonState[12]); //12
	XInput.setButton(BUTTON_Y, lastButtonState[11]);//11

	XInput.setButton(BUTTON_LB, lastButtonState[1]); //Good
	XInput.setButton(BUTTON_RB, lastButtonState[10]); //10

	XInput.setButton(BUTTON_BACK, lastButtonState[7]); //Good
	XInput.setButton(BUTTON_START, lastButtonState[6]); //Good
  XInput.setButton(BUTTON_LOGO, lastButtonState[8]); //Good

	XInput.setButton(BUTTON_L3, lastButtonState[0]); //Good
	XInput.setButton(BUTTON_R3, lastButtonState[9]); //

  XInput.setDpad(lastButtonState[2], lastButtonState[4], lastButtonState[5], lastButtonState[3]);
  XInput.send();
}

void i2cRumble(int value){
  Wire.beginTransmission(controllerAddr);
    Wire.write(rumble);
    Wire.write(value);
    Wire.endTransmission();
}

void joystickInput(){ //Read joysticks and compare to LUT's . Read triggers and scale output
  int var = analogRead(leftY) / (float)scaleFactor;
  XInput.setJoystickY(JOY_LEFT, leftYLUT[var]);

  var = analogRead(leftX) / (float)scaleFactor;
  XInput.setJoystickX(JOY_LEFT, leftXLUT[var]);
  
  var = i2c_Request(controllerAddr, yAxis) / (float)scaleFactor;
  if(var != 0){
    XInput.setJoystickY(JOY_RIGHT, rightYLUT[var]);
  }
  
  var = i2c_Request(controllerAddr, xAxis) / (float)scaleFactor;
  if(var != 0){
    XInput.setJoystickX(JOY_RIGHT, rightXLUT[var]);
  }

  
  if(analogTriggers){
    var = i2c_Request(controllerAddr, zAxis);
    if(var != 0){
      var = triggerScale(var, minR2, maxR2);
      XInput.setTrigger(TRIGGER_RIGHT, var);
    }

    var = analogRead(leftZ);
    //Serial.println(var);
    var = triggerScale(var, minL2, maxL2);
    XInput.setTrigger(TRIGGER_LEFT, var);
  } else {
    var = i2c_Request(controllerAddr, zAxis);
    if(var != 0){
      var = triggerScale(var, minR2, maxR2);
      if(var > 0){
        XInput.setButton(TRIGGER_RIGHT, 1);
      } else {
        XInput.setButton(TRIGGER_RIGHT, 0);
      }
    }
    var = analogRead(leftZ);
    var = triggerScale(var, minL2, maxL2);
    if(var > 0){
      XInput.setButton(TRIGGER_LEFT, 1);
    } else {
      XInput.setButton(TRIGGER_LEFT, 0);
    }
  }
}

void buttonRead(){ //Read button inputs and set state arrays.
  for (int i = 0; i < buttonCount; i++){
    int input = !digitalRead(buttonPins[i]);
    if (input != lastButtonState[i]){
      lastButtonState[i] = input;
    }
  }
  unsigned int buttonReq = i2c_Request(controllerAddr, buttons);
  //Serial.println(buttonReq);
  for (int i = 0; i < i2cButtonCount; i++){
    if(buttonReq & 0b00000001 == 1){
      lastButtonState[i+buttonCount] = 1; //Pressed
    } else {
      lastButtonState[i+buttonCount] = 0; //Un-Pressed
    }
    buttonReq = buttonReq >> 1;
  }
}

int triggerScale(int value, int min, int max){  //Trigger scaling
  if(value < min + triggerDeadband){
    value = min;
  }
  if(value > max - triggerDeadband){
    value = max;
  }
  
  return map(value, min, max, 0, 254);
}

int i2c_Request(int adr, int i2c_Reg){ //Controller i2c request
  Wire.beginTransmission(adr);
  Wire.write(i2c_Reg);
  Wire.endTransmission();
  Wire.requestFrom(adr, 2);
  while (Wire.available()) {
    int readingLow = Wire.read();
    int readingHigh = Wire.read();
    return(word(readingHigh, readingLow));
  }
  return 0;
}

void eepromLoad(){ //Loads stored settings from EEPROM
  if(readIntFromEEPROM(1) != -1){ //Check Joystick Calibration in EEPROM is not Empty
    readCalibrationData(); //Load joystick calibration from EEPROM
  }
}

void writeIntIntoEEPROM(int address, int number){ //Splits Int into BYTES for EEPROM
  byte byte1 = number >> 8;
  byte byte2 = number & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
}

int readIntFromEEPROM(int address){ //Converts BYTES to INT from EEPROM
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void readCalibrationData(){ //Read calibration data from EEPROM
//Left X
  minLeftX = readIntFromEEPROM(1);
  maxLeftX = readIntFromEEPROM(3);
  midLeftX = readIntFromEEPROM(5);
//Left Y
  minLeftY = readIntFromEEPROM(7);
  maxLeftY = readIntFromEEPROM(9);
  midLeftY = readIntFromEEPROM(11);
//Right Y
  minRightY = readIntFromEEPROM(13);
  maxRightY = readIntFromEEPROM(15);
  midRightY = readIntFromEEPROM(17);
// Right X
  minRightX = readIntFromEEPROM(19);
  maxRightX = readIntFromEEPROM(21);
  midRightX = readIntFromEEPROM(23);
// L2
  minL2 = readIntFromEEPROM(25);
  maxL2 = readIntFromEEPROM(27);
// R2
  minR2 = readIntFromEEPROM(29);
  maxR2 = readIntFromEEPROM(31);
}

void writeCalibrationData(){ //Store calibration data in EEPROM
//Left X
  writeIntIntoEEPROM(1, minLeftX);
  writeIntIntoEEPROM(3, maxLeftX);
  writeIntIntoEEPROM(5, midLeftX);
//Left Y
  writeIntIntoEEPROM(7, minLeftY);
  writeIntIntoEEPROM(9, maxLeftY);
  writeIntIntoEEPROM(11, midLeftY);
//Right Y
  writeIntIntoEEPROM(13, minRightY);
  writeIntIntoEEPROM(15, maxRightY);
  writeIntIntoEEPROM(17, midRightY);
// Right X
  writeIntIntoEEPROM(19, minRightX);
  writeIntIntoEEPROM(21, maxRightX);
  writeIntIntoEEPROM(23, midRightX);
// L2
  writeIntIntoEEPROM(25, minL2);
  writeIntIntoEEPROM(27, maxL2);
// R2
  writeIntIntoEEPROM(29, minR2);
  writeIntIntoEEPROM(31, maxR2);
//Commit Changes
  //EEPROM.commit();
}

void lutScaleFactor(){
  int largestValue = 0;
  if(maxLeftX > largestValue){
    largestValue = maxLeftX;
  }
  if(maxLeftY > largestValue){
    largestValue = maxLeftY;
  }
  if(maxRightX > largestValue){
    largestValue = maxRightX;
  }
  if(maxRightY > largestValue){
    largestValue = maxRightY;
  }
  scaleFactor = largestValue / (float)lutLength;
}

void buildLUTs(){ //Builds the look up table for each joystick Axis
  lutScaleFactor(); //Calulate Scale Factor for LUT's
  //Serial.print("Scale Factor = ");
  //Serial.println(scaleFactor);
  //Left X
  for(int i = 0; i < lutLength; i++){
    leftXLUT[i] = joystickScale(i * scaleFactor, minLeftX, midLeftX, maxLeftX);
    //Serial.print("Input : ");
    //Serial.print(i);
    //Serial.print(" Output : ");
    //Serial.println(leftXLUT[i]);
  }
  //Left Y
  for(int i = 0; i < lutLength; i++){
    leftYLUT[i] = joystickScale(i * scaleFactor, minLeftY, midLeftY, maxLeftY);
  }
  //Right X
  for(int i = 0; i < lutLength; i++){
    rightXLUT[i] = joystickScale(i * scaleFactor, minRightX, midRightX, maxRightX);
  }
  //Right Y
  for(int i = 0; i < lutLength; i++){
    rightYLUT[i] = joystickScale(i * scaleFactor, minRightY, midRightY, maxRightY);
  }
}

int joystickScale(int value, int min, int mid, int max){
  int output;
  if(value < mid - deadBand){ //Value is less than halfway and outside deadband
    output = map(value, min + earlyStop, mid, 0, scaleMax / 2);
    if(output < 0){
      output = 0;
    }
  } else if(value > mid + deadBand){ //Value is greater than halfway and outside deadband
    output = map(value, mid, max - earlyStop, scaleMax / 2, scaleMax);
    if(output > scaleMax){
      output = scaleMax;
    }
  } else { //Value is within deadband.
    output = scaleMax / 2;
  }
  return output;
}

int clipJoystickOutput(int value){
  value = value / (float)scaleFactor;
  if(value > scaleMax){
    return scaleMax;
  }
  return value;
}

void joystickCalibration(){ //Joystick Calibration Sequence
  if(calibrationStep == 1){
    buttonRead();
    if(lastButtonState[13] == 1){
      midRightX = i2c_Request(controllerAddr, xAxis);
      midRightY = i2c_Request(controllerAddr, yAxis);
      midLeftX = analogRead(leftX);
      midLeftY = analogRead(leftY);
      delay(50);
      minLeftX = midLeftX;
      minLeftY = midLeftY;
      maxLeftX = 0;
      maxLeftY = 0;
      minRightX = midRightX;
      minRightY = midRightY;
      maxRightX = 0;
      maxRightY = 0;
      calibrationStep = 2;
      digitalWrite(rumbleLarge, HIGH);
      delay(100);
      digitalWrite(rumbleLarge, LOW);
      //Serial.printLn("calibrationStepTwo");
      delay(150);
    }
  } else if(calibrationStep == 2){
    int var = analogRead(leftX);
    if(var != 0){
      if(var > maxLeftX) maxLeftX = var;
      if(var < minLeftX) minLeftX = var;
    }
    var = analogRead(leftY);
    if(var != 0){
      if(var > maxLeftY) maxLeftY = var;
      if(var < minLeftY) minLeftY = var;
    }
    var = i2c_Request(controllerAddr, xAxis);
    if(var != 0){
      if(var > maxRightX) maxRightX = var;
      if(var < minRightX) minRightX = var;
    }
    var = i2c_Request(controllerAddr, yAxis);
    if(var != 0){
      if(var > maxRightY) maxRightY = var;
      if(var < minRightY) minRightY = var;
    }
    buttonRead();
    if(lastButtonState[13] == 1){
      calibrationStep = 3;
      minL2 = 1023;
      maxL2 = 0;
      minR2 = 1023;
      maxR2 = 0;
      //Serial.println("Calibrate triggers");
      digitalWrite(rumbleLarge, HIGH);
      delay(100);
      digitalWrite(rumbleLarge, LOW);
      delay(150);
    }
  } else if(calibrationStep == 3){
    int var = i2c_Request(controllerAddr, zAxis); //R2
    if(var != 0){
      if(var > maxR2) maxR2 = var;
      if(var < minR2) minR2 = var;
    }
    var = analogRead(leftZ); //L2
    if(var != 0){
      if(var > maxL2) maxL2 = var;
      if(var < minL2) minL2 = var;
    }
    if(lastButtonState[13] == 1){ //Complete Calibration
      digitalWrite(rumbleLarge, HIGH);
      delay(100);
      digitalWrite(rumbleLarge, LOW);
      delay(150);
      writeCalibrationData(); //Update EEPROM
      buildLUTs();
      delay(50);
      calibrationStep = 1;
      calibrationMode = false;
    }
  }
}

void xInputCallback(uint8_t packetType) {
	if (packetType == (uint8_t) XInputReceiveType::LEDs) {
		return;
	}

	else if (packetType == (uint8_t) XInputReceiveType::Rumble) {
		rumbleLeft = XInput.getRumbleLeft() / 4; 
    rumbleRight = XInput.getRumbleRight();
	}
}

void pwm613configure(int mode) {
  TCCR4A=0;// TCCR4A configuration
  TCCR4B=mode;// TCCR4B configuration
  TCCR4C=0;// TCCR4C configuration
  TCCR4D=0;// TCCR4D configuration
  TCCR4D=0;// TCCR4D configuration

  // PLL Configuration
  // Use 96MHz / 2 = 48MHz
  PLLFRQ=(PLLFRQ&0xCF)|0x30;
  // PLLFRQ=(PLLFRQ&0xCF)|0x10; // Will double all frequencies

  OCR4C=255;// Terminal count for Timer 4 PWM
}

void pwmSet6(int value) {
  OCR4D=value;   // Set PWM value
  DDRD|=1<<7;    // Set Output Mode D7
  TCCR4C|=0x09;  // Activate channel D
}

