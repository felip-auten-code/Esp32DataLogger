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
#include <PubSubClient.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>


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
void rec(int n_bytes);
void TaskReceiveAndSave(void *param);
void checkSDCard();
void setupI2C_STM32();

TaskHandle_t ReceiveAndSave;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  checkSDCard();

  xTaskCreatePinnedToCore(  TaskReceiveAndSave, 
                            "TaskReceiveAndSave", 
                            4096, 
                            NULL, 
                            3, 
                            &ReceiveAndSave, 
                            1);

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
    sprintf(path, "/Logs/Work%d.txt", NUMB);                // Nome do arquivo - random
    sprintf(name, "WORK: %d \n%s \n", NUMB, DataFromST);    // Configurações importantes -> Primeira linha do arquyivo
    CURRENT_WORK_NUMB = NUMB;
    flagFileCreated=true;
    start_millis = millis();
    writeFile(SD, path, name);
  }
}

void TaskReceiveAndSave(void *param){
  
  char path[30] = {0}, name[50]={0}, time[10];
  while(1){
    if(flagDataFrame){                                          // Chegou um frame de informação na porta i2c
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
    if(flagConfsReceived){                                      // Recebeu o Frame de início do trabalho
      flagConfsReceived = false;
      uint32_t NUMB = esp_random();
      sprintf(path, "/Logs/Work%d.txt", NUMB);                  // Nome do arquivo - random
      sprintf(name, "WORK: %d \n%s \n", NUMB, DataFromST);      // Configurações importantes -> Primeira linha do arquyivo
      CURRENT_WORK_NUMB = NUMB;
      flagFileCreated=true;
      start_millis = millis();
      writeFile(SD, path, name);
    }
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

void rec(int n_bytes){              // Recebe stream de dados da porta I2C e Guarda em String
  while(Wire.available()){
    char c = Wire.read();
    if(c != '$' && c != '&' && c != '#'){         // Acumunla informação
      DataFromST[indexData] = c;
      indexData++;
    }
    if(c == '#' ){                                // Final do Frame de excecução
      //DataFromST[indexData] = '\n';
      indexData = 0;
      //Serial.println(DataFromST);
      flagDataFrame = true;
    }else if( c == '*'){                          // deve criar novo arquivo com o novo trablaho
      DataFromST[indexData] = '\n';
      indexData = 0;
      flagConfsReceived = true;
    }
  }
}

void SaveFrame(String in){
  
}

void checkSDCard(){
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

}

void setupI2C_STM32(){
  Wire.setBufferSize(526);
  Wire.begin(30);
  Wire.onReceive(rec);
}