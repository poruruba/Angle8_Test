#include <M5StickC.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "M5_ANGLE8.h"

M5_ANGLE8 angle8;

#define DISCONNECT_WAIT 3000

#define UUID_SERVICE "08030900-7d3b-4ebf-94e9-18abc4cebede"
#define UUID_WRITE "08030901-7d3b-4ebf-94e9-18abc4cebede"
#define UUID_READ "08030902-7d3b-4ebf-94e9-18abc4cebede"
#define UUID_NOTIFY "08030903-7d3b-4ebf-94e9-18abc4cebede"

#define BIT_BTN_A   (0x0001 << 0)
#define BIT_BTN_B   (0x0001 << 1)
#define BIT_BTN_C   (0x0001 << 3)
#define BIT_ANGLE_1   (0x0001 << 4)
#define BIT_ANGLE_2   (0x0001 << 5)
#define BIT_ANGLE_3   (0x0001 << 6)
#define BIT_ANGLE_4   (0x0001 << 7)
#define BIT_ANGLE_5   (0x0001 << 8)
#define BIT_ANGLE_6   (0x0001 << 9)
#define BIT_ANGLE_7   (0x0001 << 1)
#define BIT_ANGLE_8   (0x0001 << 11)

#define ANGLE_MARGIN  5

BLECharacteristic *pCharacteristic_write;
BLECharacteristic *pCharacteristic_read;
BLECharacteristic *pCharacteristic_notify;
BLEAdvertising *g_pAdvertising = NULL;

#define UUID_VALUE_SIZE 20
uint8_t value_buffer[UUID_VALUE_SIZE] = { 0 };

bool isConnected = false;

void readValue(void);
void sendBuffer(void);

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    isConnected = true;
    Serial.println("Connected\n");
  }

  void onDisconnect(BLEServer* pServer){
    isConnected = false;

    BLE2902* desc = (BLE2902*)pCharacteristic_notify->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(false);

    Serial.println("Disconnected\n");

    g_pAdvertising->stop();
    delay(DISCONNECT_WAIT);
    g_pAdvertising->start();
  }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic* pCharacteristic){
    Serial.print("onWrite");
    uint8_t* value = pCharacteristic->getData();
    std::string str = pCharacteristic->getValue(); 
    int recv_len = str.length();
    Serial.printf("(%d)\n", recv_len);

    if( recv_len >= 5 ){
      uint32_t color = (value[2] << 16) | (value[3] << 8) | value[4];
      angle8.setLEDColor(value[0], color, value[1]);
    }
  }
/*
  void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code){
  }
  void BLECharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic){
  };
*/
};

void taskServer(void*) {
  BLEDevice::init("Angle8BleCustom");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  BLEService *pService = pServer->createService(UUID_SERVICE);

  pCharacteristic_write = pService->createCharacteristic( UUID_WRITE, BLECharacteristic::PROPERTY_WRITE );
  pCharacteristic_write->setAccessPermissions(ESP_GATT_PERM_WRITE);
  pCharacteristic_write->setCallbacks(new MyCharacteristicCallbacks());

  pCharacteristic_read = pService->createCharacteristic( UUID_READ, BLECharacteristic::PROPERTY_READ );
  pCharacteristic_read->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristic_read->setValue(value_buffer, sizeof(value_buffer));

  pCharacteristic_notify = pService->createCharacteristic( UUID_NOTIFY, BLECharacteristic::PROPERTY_NOTIFY );
  pCharacteristic_notify->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
  pCharacteristic_notify->addDescriptor(new BLE2902());

  pService->start();

  g_pAdvertising = pServer->getAdvertising();
  g_pAdvertising->addServiceUUID(UUID_SERVICE);
  g_pAdvertising->start();

  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}

void setup() {
  // put your setup code here, to run once:
  M5.begin(true, true, true);
  Serial.printf("setup start\n");

  Wire.begin(32, 33);

  while (!angle8.begin(ANGLE8_I2C_ADDR)) {
    Serial.println("angle8 Connect Error");
    delay(100);
  }

  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);

  Serial.printf("setup finished\n");
}

void loop() {
  // put your main code here, to run repeatedly:
  M5.update();

  readValue();

  bool changed = false;

  static uint8_t buttons[3] = { 0 };

  for( uint8_t i = 0 ; i < 3 ; i++ ){
    if( value_buffer[i] != buttons[i] ){
      buttons[i] = value_buffer[i];
      changed = true;
      if( buttons[i] )
        Serial.printf("Btn(%d) Pressed\n", i);
      else
        Serial.printf("Btn(%d) Released\n", i);
    }    
  }

  static uint8_t values[8] = { 0 };
  for( uint8_t i = 0 ; i < 8 ; i++ ){
    if( abs( value_buffer[3 + i] - values[i]) >= ANGLE_MARGIN ){
      values[i] = value_buffer[3 + i];
      changed = true;
      Serial.printf("Angle(%d) Changed\n", i);
    }
  }

  if( changed ){
    sendBuffer();
  }

  delay(1);
}

void readValue(void){
  value_buffer[0] = M5.BtnA.isPressed() ? 0x01 : 0x00;
  value_buffer[1] = M5.BtnB.isPressed() ? 0x01 : 0x00;
  value_buffer[2] = angle8.getDigitalInput() ? 0x01 : 0x00;
  for( uint8_t i = 0 ; i < 8 ; i++ ){
    value_buffer[3 + i] = (uint8_t)angle8.getAnalogInput(i, _8bit);
  }

  if( isConnected )
    pCharacteristic_read->setValue(value_buffer, 11);
}

void sendBuffer(void){
  Serial.println("SendBuffer");

  if( !isConnected )
    return;

  BLE2902* desc = (BLE2902*)pCharacteristic_notify->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if( !desc->getNotifications() )
    return;

  pCharacteristic_notify->setValue(value_buffer, 11);
  pCharacteristic_notify->notify();
}