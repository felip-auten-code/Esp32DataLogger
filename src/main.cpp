/** 
 PLACA ESP32 38 PINS

      COMM SPI - SD CARD - VSPI
  #   MOSI - GPIO 23
  #   MISO - CPIO 19
  #   CLK  - GPIO 18
  #   CS   - GPIO 5

      COM I2C - STM32F407 
  #   SDA - GPIO 21
  #   SCL - GPIO 22

*/

// FRAME De CONFIG
// "&TempoEspera-ModoOP*"

// FRAME DE TEMPO DE EXCEC
// 
// "$AlturaIdeal-LE-LI-RE-RI-MargBottomLeft-MargTopLeft-MargBottomRight-MargTopRight-ActionLeft-ACtionRight%"

#include <Arduino.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Ticker.h>
#include <time.h>
#include <iostream>
#include <String>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sdcard.h"
#include "esp_system.h"
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x12     // I2C address of the STM32F4
#define MAX_DATA_LENGTH 200        // Length of data expected from STM32F4
#define DEBUG_SDCARD true
#define I2C_SLAVE_SDA_IO GPIO_NUM_21
#define I2C_SLAVE_SCL_IO GPIO_NUM_22
//#define I2C_BUFFER_LENGTH 512

char  receivedData[MAX_DATA_LENGTH + 1];  // Buffer for storing received data (+1 for null terminator)
bool  dataAvailable = false;    
char  DataFromST[MAX_DATA_LENGTH + 1], ConfsFromST[MAX_DATA_LENGTH + 1];
int   indexData, indexConfs;
int   AlturaIdeal, LE, LI, RE, RI, MBL, MTL, MBR, MTR, AC_L, AC_R;
bool  flagDataFrame=0, flagConfsReceived=0, flagFileCreated=0;
uint32_t start_millis=0;

uint32_t  CURRENT_WORK_NUMB;
String    CURRENT_WORK_PATH;

// put function declarations here:
void ScanI2CAddrs();
void receiver(int n_bytes);
void rec(int n_bytes);
void requestDataFromSTM32();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  File logs = SD.open("/Logs","r", true);
  if(logs){
    Serial.println("yes logs");
  }else{
    Serial.println("not logs");
    createDir(SD, "/Logs");
  }
  logs = SD.open("/Logs","r", true);
  if(logs){
    Serial.println("yes logs");
  }else{
    Serial.println("not logs");
  }
  listDir(SD, "/", 3);

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

  Wire.setBufferSize(526);
  Wire.begin(30);
  Wire.onReceive(rec);

  //ScanI2CAddrs(); 
}

void loop() {
  // put your main code here, to run repeatedly:
  char path[30] = {0}, name[50]={0}, time[10];
  if(flagDataFrame){                                // Chegou um frame de informação na porta i2c
    flagDataFrame = false;
    Serial.println(flagFileCreated);
    if(flagFileCreated){                                      // Caso o arquivo ja tenha sido criado -> adicione as linhas
      sprintf(path, "/Logs/Work%d.txt", CURRENT_WORK_NUMB);
      sprintf(DataFromST, "%s-%s\n", DataFromST, itoa(millis()-start_millis, time, 10 ));
      appendFile(SD, path, DataFromST);
      Serial.println(path);
    }
    memset(DataFromST, 0, sizeof(DataFromST));                // Clean Buffer
    //delay(50);
  } 
  if(flagConfsReceived){                            // Recebeu o Frame de início do trabalho
    flagConfsReceived = false;
    uint32_t NUMB = esp_random();
    sprintf(path, "/Logs/Work%d.txt", NUMB);
    sprintf(name, "WORK: %d \n%s \n", NUMB, DataFromST);
    CURRENT_WORK_NUMB = NUMB;
    flagFileCreated=true;
    start_millis = millis();
    writeFile(SD, path, name);
  }
}

// put function definitions here:
void ScanI2CAddrs(){
    byte error, addr;
    int nDevices=0;

    for(addr = 1; addr < 127; addr++){
        Wire.beginTransmission(addr);
        error = Wire.endTransmission();
        Serial.printf("i: %d \t error: %d \n", addr, error);
        if(error == 0){
            Serial.print("Device Found at: 0x");
            Serial.print(addr, HEX);
            Serial.print("\n");
        }
        else if (error==4)
        {
            Serial.print("Unknown error at address 0x");
            if (addr<16)
                Serial.print("0");
            Serial.println(addr,HEX);
        }
    }
}

void rec(int n_bytes){
  while(Wire.available()){
    char c = Wire.read();
    if(c != '$' && c != '&' && c != '#'){
      DataFromST[indexData] = c;
      indexData++;
    }
    if(c == '#' ){
      //DataFromST[indexData] = '\n';
      indexData = 0;
      //Serial.println(DataFromST);
      flagDataFrame = true;
    }else if( c == '*'){                  // deve criar novo arquivo com o novo trablaho
      DataFromST[indexData] = '\n';
      indexData = 0;
      flagConfsReceived = true;
    }
  }
}

void SaveFrame(String in){
  
}

void receiver(int n_bytes){
  while(Wire.available()){
    char c = Wire.read();
    int excec=0;

  if(excec == 1){
    DataFromST[indexData] = c;
    indexData++;
  }else if(excec == 99){
    ConfsFromST[indexConfs] = c;
    indexConfs++;
  }


    if(c == '$'){               // Start of Frame
      indexData = 0;
      memset(DataFromST, 0, sizeof(DataFromST));
      excec = 1;
    }else if(c == '#'){         // End of Frame
      indexData = 0;
      flagDataFrame = true;

    }else if(c == '&'){         // Start of Confs
      excec = 99;
      indexConfs = 0;
      memset(ConfsFromST, 0, sizeof(ConfsFromST));
    }else if(c == '*'){         // End of Confs
      flagConfsReceived = true;
      indexConfs = 0;
    }
    //Serial.printf("RECEIVED: %c \n", c);
  }
  delay(250);
}

void requestDataFromSTM32() {
  // Request data from the STM32F4 without blocking
  Wire.requestFrom(I2C_SLAVE_ADDRESS, MAX_DATA_LENGTH);

  int index = 0;
  while (Wire.available()) {
    if (index < MAX_DATA_LENGTH) {
      receivedData[index] = Wire.read();  // Read each byte and store it in the buffer
      Serial.print("RECEIIVED:  ");
      Serial.println(receivedData[index]);
      index++;
      
    }
  }

  receivedData[index] = '\0';  // Null-terminate the string

  // If data was received, set the flag to process it
  if (index > 0) {
    dataAvailable = true;
  }
}