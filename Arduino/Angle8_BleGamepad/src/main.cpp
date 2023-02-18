#include <M5StickC.h>
#include <Wire.h>
#include <BleGamepad.h>
#include "M5_ANGLE8.h"

#define ANGLE_MARGIN  5

BleGamepad bleGamepad("Angle8BleGamepad", "MyHome", 100);
M5_ANGLE8 angle8;

void setup() {
  // put your setup code here, to run once:
  M5.begin(true, true, true);
  Serial.println("setup start");

  Wire.begin(32, 33);

  while (!angle8.begin(ANGLE8_I2C_ADDR)) {
    Serial.println("angle8 Connect Error");
    delay(100);
  }

  BleGamepadConfiguration config;
  config.setButtonCount(3);
  config.setHatSwitchCount(0);
  config.setWhichAxes(true, true, true, true, true, true, true, true);
  config.setAxesMax(0xFFF);
  config.setAxesMin(0);
  bleGamepad.begin(&config);

  Serial.println("setup finished");
}

void loop() {
  // put your main code here, to run repeatedly:
  M5.update();

  static bool buttons[3] = { 0 };
  if( M5.BtnA.isPressed() ){
    if( !buttons[0] ){
      buttons[0] = true;
      bleGamepad.press(1);
      Serial.printf("BtnA Pressed\n");
    }
  }else{
    if( buttons[0] ){
      buttons[0] = false;
      bleGamepad.release(1);
      Serial.printf("BtnA Released\n");
    }
  }
  if( M5.BtnB.isPressed() ){
    if( !buttons[1] ){
      buttons[1] = true;
      bleGamepad.press(2);
      Serial.printf("BtnB Pressed\n");
    }
  }else{
    if( buttons[1] ){
      buttons[1] = false;
      bleGamepad.release(2);
      Serial.printf("BtnB Released\n");
    }
  }
  if( buttons[2] != angle8.getDigitalInput() ){
    buttons[2] = !buttons[2];
    if( buttons[2])
      bleGamepad.press(3);
    else
      bleGamepad.release(3);
    if( buttons[2] )
      Serial.printf("Switch On\n");
    else
      Serial.printf("Switch Off\n");
  }


  static uint16_t values[8] = { 0 };
  bool changed = false;
  for( uint8_t i = 0 ; i < 8 ; i++ ){
    uint16_t value = angle8.getAnalogInput(i, _12bit);
    if( abs(value - values[i] ) >= ANGLE_MARGIN ){
      values[i] = value;
      changed = true;

    }
  }
  if( changed ){
    bleGamepad.setAxes(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
  }

  delay(1);
}