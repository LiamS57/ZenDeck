#include "IQS5xx.h"
#include "defs.h"
#include "I2C.h"
#include <EEPROM.h>

#include "HID-Project.h"
//#include "Mouse.h"

#include <APA102.h>

//LED Setup
const uint8_t dataPin = 8;
const uint8_t clockPin = 9;
APA102<dataPin, clockPin> ledStrip;
const uint16_t ledCount = 20;
rgb_color colors[ledCount];


//Volume Buttons
byte volUp = 4;
byte volDn = 12;

//User Configurable LED Settings
byte primaryColour[3] = {50,50,50}; //Red, Green, Blue
byte secondaryColour[3] = {150,0,0};
int animationSpeed = 40;
byte ledMode = 3; //Halo = 0, Breath = 1, Bands = 2, Rainbow = 3, Solid = 4,
byte ledBrightness = 1;

//LED Sequence Variables
float redScale;
float greenScale;
float blueScale;
int sequenceStep = 0;
unsigned long ledTimer;

//LED Breath Variables
bool breathIncrease = true;

//LED Halo Variables
int haloCount = 0; //Which is the brightest LED in the sequence

//LED Bands
bool evenNumber = false;
int loopCounter;

//Trackpad
uint8_t 	Data_Buff[44];
uint16_t	ui16SnapStatus[15], ui16PrevSnap[15];
bool trackpadEnabled = true;
unsigned long tapTimer = 0;
bool rightClick = false;
int trackpadDisableDelay = 200;

//Serial Comms
bool stringComplete = false;
bool incomingSetting = false;
byte targetValue;
byte inputValue;
byte inputArray[4];
int serialCounter;

//Serial Value Codes
byte pRed = 0x01;
byte pGreen = 0x02;
byte pBlue = 0x03;
byte sRed = 0x04;
byte sGreen = 0x05;
byte sBlue = 0x06;
byte speed = 0x07;
byte brightness = 0x08;
byte mode = 0x09;
byte confirmationCode = 0x6B;
byte commitChanges = 0x77;

void setup(){
  Serial.begin(9600);
  eepromLoad();

  Mouse.begin();
  Consumer.begin();

  //while(!Serial);

	pinMode(RDY_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(RDY_PIN), readTrackpad, HIGH);

  pinMode(volUp, INPUT_PULLUP);
  pinMode(volDn, INPUT_PULLUP);

	I2C_Setup();
	IQS5xx_AcknowledgeReset();
	IQS5xx_CheckVersion();
	Close_Comms();

  calculateColourMultiplier();
}


void loop() {
  serialRead();
  
  if(ledTimer + animationSpeed < millis()){
    if(ledMode == 0){ //Halo LED
      haloLED();
    } else if(ledMode == 1){ //Breath LED
      breathLED();
    } else if(ledMode == 2){ //LED Bands
      ledBand();
    } else if(ledMode == 3){
      rainbowLED();
    } else if(ledMode == 4){
      solidLED();
    }
    ledTimer = millis();
  }
  if(digitalRead(RDY_PIN)){
    readTrackpad();
  }
  if(digitalRead(volUp) == LOW){
    Consumer.press(MEDIA_VOLUME_UP);
    delay(20);
    Consumer.release(MEDIA_VOLUME_UP);
  }
  if(digitalRead(volDn) == LOW){
    Consumer.press(MEDIA_VOLUME_DOWN);
    delay(20);
    Consumer.release(MEDIA_VOLUME_DOWN);
  }
  if(tapTimer != 0 && tapTimer + trackpadDisableDelay < millis()){
    tapTimer = 0;
    if(trackpadEnabled){
      Mouse.press(MOUSE_RIGHT);
      delay(10);
      Mouse.release(MOUSE_RIGHT);
    }
  }
}

void readSerialInput(){
  
  if (stringComplete) {
    colors[10] = rgb_color(255,0,0);
    ledStrip.write(colors, ledCount, ledBrightness);

    if(targetValue == confirmationCode){
      Serial.write(confirmationCode);
    } else if(targetValue == pRed){
      if(incomingSetting){
        primaryColour[0] = inputValue;
      } else {
        Serial.write(primaryColour[0]);
      }
    } else if(targetValue == pGreen){
      if(incomingSetting){
        primaryColour[1] = inputValue;
      } else {
        Serial.write(primaryColour[1]);
      }
    } else if(targetValue == pBlue){
      if(incomingSetting){
        primaryColour[2] = inputValue;
      } else {
        Serial.write(primaryColour[2]);
      }
    } else if(targetValue == sRed){
      if(incomingSetting){
        secondaryColour[0] = inputValue;
      } else {
        Serial.write(secondaryColour[0]);
      }
    } else if(targetValue == sGreen){
      if(incomingSetting){
        secondaryColour[1] = inputValue;
      } else {
        Serial.write(secondaryColour[1]);
      }
    } else if(targetValue == sBlue){
      if(incomingSetting){
        secondaryColour[2] = inputValue;
      } else {
        Serial.write(secondaryColour[2]);
      }
    } else if(targetValue == speed){
      if(incomingSetting){
        animationSpeed = inputValue;
      } else {
        Serial.write(animationSpeed);
      }
    } else if(targetValue == mode){
      if(incomingSetting){
        ledMode = inputValue;
      } else {
        Serial.write(ledMode);
      }
    } else if(targetValue == brightness){
      if(incomingSetting){
        ledBrightness = inputValue;
      } else {
        Serial.write(ledBrightness);
      }
    } else if(targetValue == commitChanges){
      eepromUpdate();
    }
    stringComplete = false;
    incomingSetting = false;
  }
}

void calculateColourMultiplier(){
  int steps;
  if(ledMode == 0){
    steps = 10;
  } else if(ledMode == 1){
    steps = 100;
  } else if(ledMode == 2){
    steps = 10;
  }
  redScale = primaryColour[0] - secondaryColour[0];
  redScale = redScale / steps; //Result 1.5

  greenScale = primaryColour[1] - secondaryColour[1];
  greenScale = greenScale / steps; //Result 0

  blueScale = primaryColour[2] - secondaryColour[2];
  blueScale = blueScale / steps; //Result -5
}

void LED(){
  uint8_t time = millis() >> 2;
  for(uint16_t i = 0; i < ledCount; i++)
  {
    uint8_t x = time - i * 8;
    colors[i] = rgb_color(x, 255 - x, x);
  }

  ledStrip.write(colors, ledCount, ledBrightness);
}

void solidLED(){
  for(int i = 0; i < ledCount; i++){
    colors[i] = rgb_color(primaryColour[0], primaryColour[1], primaryColour[2]);
  }
  ledStrip.write(colors, ledCount, ledBrightness);
}

void haloLED(){
  for(int i = 0; i < 10; i++){
    if(i == haloCount){
      sequenceStep = 0;
    }
    colors[i] = rgb_color(primaryColour[0] - (redScale * sequenceStep),primaryColour[1] - (greenScale * sequenceStep),primaryColour[2] - (blueScale * sequenceStep));
    colors[i+10] = rgb_color(primaryColour[0] - (redScale * sequenceStep),primaryColour[1] - (greenScale * sequenceStep),primaryColour[2] - (blueScale * sequenceStep));
    sequenceStep++;
  }

  if(haloCount < 10){
    haloCount++;
  } else {
    haloCount = 0;
  }

  ledStrip.write(colors, ledCount, ledBrightness);
}

void ledBand(){
  if(loopCounter == 20){
    if(evenNumber){
      evenNumber = false;
    } else {
      evenNumber = true;
    }
    loopCounter = 0;
  }

  for(int i = 0; i < ledCount; i++){
    if(evenNumber){
      if(i % 2){
        colors[i] = rgb_color(primaryColour[0],primaryColour[1],primaryColour[2]);
      } else {
        colors[i] = rgb_color(secondaryColour[0],secondaryColour[1],secondaryColour[2]);
      }
    } else {
      if(i % 2){
        colors[i] = rgb_color(secondaryColour[0],secondaryColour[1],secondaryColour[2]);
      } else {
        colors[i] = rgb_color(primaryColour[0],primaryColour[1],primaryColour[2]);
      }
    }
  }
  ledStrip.write(colors, ledCount, ledBrightness);
  loopCounter++;
}

void breathLED(){
  if(breathIncrease){
    if(sequenceStep < 100){
      sequenceStep++;
    } else {
      breathIncrease = false;
    }
  } else {
    if(sequenceStep > 0){
      sequenceStep--;
    } else {
      breathIncrease = true;
    }
  }
  for(int i = 0; i < ledCount; i++){
    colors[i] = rgb_color(primaryColour[0] - (redScale * sequenceStep),primaryColour[1] - (greenScale * sequenceStep),primaryColour[2] - (blueScale * sequenceStep));
  }
  ledStrip.write(colors, ledCount, ledBrightness);
}

void rainbowLED(){
  uint8_t time = millis() >> 4;

  for(uint16_t i = 0; i < ledCount; i++)
  {
    uint8_t p = time - i * animationSpeed;
    colors[i] = hsvToRgb((uint32_t)p * 359 / 256, 255, 255);
  }

  ledStrip.write(colors, ledCount, ledBrightness);
}

rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return rgb_color(r, g, b);
}

void readTrackpad(){
  uint8_t		ui8TempData[30], i;
  I2C_Read(GestureEvents0_adr, &Data_Buff[0], 44);

	if((Data_Buff[3] & SNAP_TOGGLE) != 0)
	{
		I2C_Read(SnapStatus_adr, &ui8TempData[0], 30);
		for(i = 0; i < 15; i++)
		{
			ui16PrevSnap[i] = ui16SnapStatus[i];
			ui16SnapStatus[i] = ((uint16_t)(ui8TempData[2*i])<<8) + 
								 (uint16_t)ui8TempData[(2*i)+1];
		}
	}
	Close_Comms();
	Process_XY();
}

void eepromUpdate(){
  byte temp;
  temp = EEPROM.read(0);
  if(temp != primaryColour[0]){
    EEPROM.write(0, primaryColour[0]);
  }
  temp = EEPROM.read(1);
  if(temp != primaryColour[1]){
    EEPROM.write(1, primaryColour[1]);
  }
  temp = EEPROM.read(2);
  if(temp != primaryColour[2]){
    EEPROM.write(2, primaryColour[2]);
  }
  temp = EEPROM.read(3);
  if(temp != secondaryColour[0]){
    EEPROM.write(3, secondaryColour[0]);
  }
  temp = EEPROM.read(4);
  if(temp != secondaryColour[1]){
    EEPROM.write(4, secondaryColour[1]);
  }
  temp = EEPROM.read(5);
  if(temp != secondaryColour[2]){
    EEPROM.write(5, secondaryColour[2]);
  }
  temp = EEPROM.read(6);
  if(temp != animationSpeed){
    EEPROM.write(6, animationSpeed);
  }
  temp = EEPROM.read(7);
  if(temp != ledMode){
    EEPROM.write(7, ledMode);
  }
  temp = EEPROM.read(8);
  if(temp != ledBrightness){
    EEPROM.write(8, ledBrightness);
  }
}

void eepromLoad(){
  primaryColour[0] = EEPROM.read(0);
  primaryColour[1] = EEPROM.read(1);
  primaryColour[2] = EEPROM.read(2);
  
  secondaryColour[0] = EEPROM.read(3);
  secondaryColour[1] = EEPROM.read(4);
  secondaryColour[2] = EEPROM.read(5);

  animationSpeed = EEPROM.read(6);
  ledMode = EEPROM.read(7);
  ledBrightness = EEPROM.read(8);
}

void serialRead(){
  byte incomingByte;
  
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    //Serial.readBytes(inputArray[serialCounter], 1);
    inputArray[serialCounter] = incomingByte;
    serialCounter++;
  }
  if(serialCounter == 4){
    if(inputArray[1] == ','){
      incomingSetting = true;
      inputValue = inputArray[2];
    }
    targetValue = inputArray[0];
    //Serial.println(inputValue);
    serialCounter = 0;
    stringComplete = true;
    readSerialInput();
  }
}




