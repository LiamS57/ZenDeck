#include <Wire.h>
byte i2cAddr = 0x01;
byte buttons = 0xF1;
byte xAxis = 0xF2;
byte yAxis = 0xF3;
byte zAxis = 0xF4;
byte rumble = 0xF5;
byte outputMode = 0x00;
byte outputArray[2];

//Button Handling
byte buttonPins[6] = {0,1,2,3,4,17}; //B, A, X, Y, R1, R3
byte buttonCount = 6;
byte lastButtonState[6];

//Axis
byte xAxisPin = A0;
byte yAxisPin = A1;
byte zAxisPin = A2;
byte rumblePin = 10;
int xAxisValue;
int yAxisValue;
int zAxisValue;
int rumbleValue = 0;

void setup() {
  Wire.begin(i2cAddr);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  for(int i = 0; i < buttonCount; i++){ //Setup controller input pins
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(xAxisPin, INPUT);
  pinMode(yAxisPin, INPUT);
  pinMode(zAxisPin, INPUT);

  TCCR1B = TCCR1B & B11111000 | B00000001; // set timer 1 divisor to 1 for PWM frequency of 31372.55 Hz
  pinMode(rumblePin, OUTPUT);
}

void loop() {
  buttonRead();
  axisRead();
  rumbleMotor();
}

void buttonRead(){ //Read button inputs and set state arrays.
  for (int i = 0; i < buttonCount; i++){ 
    int input = !digitalRead(buttonPins[i]);
    if (input != lastButtonState[i]){
      lastButtonState[i] = input;
    }
  }
}

void axisRead(){
  xAxisValue = analogRead(xAxisPin);
  yAxisValue = analogRead(yAxisPin);
  zAxisValue = analogRead(zAxisPin);
}

void buttonOutput(){
  unsigned int output = 0;
  for(int i = 0; i < buttonCount; i++){
    output = output << 1;
    if(lastButtonState[i] == 1){
      output += 1;
    }
  }
  outputArray[0] = lowByte(output);
  outputArray[1] = highByte(output);
}

void rumbleMotor(){
  if(rumbleValue > 0){
    analogWrite(rumblePin, rumbleValue * 10.24); //Scale for 0-100 percent
  } else {
    digitalWrite(rumblePin, LOW);
  }
}

void requestEvent(){
  if(outputMode == buttons){
    buttonOutput();
    Wire.write(outputArray,2);
  } else if(outputMode == xAxis){
    outputArray[0] = lowByte(xAxisValue);
    outputArray[1] = highByte(xAxisValue);
    Wire.write(outputArray,2);
  } else if(outputMode == yAxis){
    outputArray[0] = lowByte(yAxisValue);
    outputArray[1] = highByte(yAxisValue);
    Wire.write(outputArray,2);
  } else if(outputMode == zAxis){
    outputArray[0] = lowByte(zAxisValue);
    outputArray[1] = highByte(zAxisValue);
    Wire.write(outputArray,2);
  }
}

void receiveEvent(int value){
  while(Wire.available()){
    if(outputMode == rumble){
      int temp = Wire.read();
      if(temp <= 100){ //If input is within range for rumble. Else revert back to normal mode
        rumbleValue = temp; //Expected values 0-100
      } else { 
        outputMode = temp;
      }
    } else {
      outputMode = Wire.read();
    }
  }
}
