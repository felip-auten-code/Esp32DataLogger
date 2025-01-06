#ifndef sdcard_h

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DEBUG_SDCARD true

//------------- SD CARD
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    #if DEBUG_SDCARD
        Serial.printf("Listing directory: %s\n", dirname);
    #endif

    File root = fs.open(dirname);

    
    if(!root){        
        #if DEBUG_SDCARD
            Serial.println("Failed to open directory");
        #endif
        return;
    }
    if(!root.isDirectory()){
        #if DEBUG_SDCARD
            Serial.println("Not a directory");
        #endif
        return;
    }
     

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            #if DEBUG_SDCARD
                Serial.print("  DIR : ");
                Serial.println(file.name());
            #endif
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            #if DEBUG_SDCARD
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.println(file.size());
            #endif
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    #if DEBUG_SDCARD
        Serial.printf("Creating Dir: %s\n", path);
    #endif

    if(fs.mkdir(path)){
        #if DEBUG_SDCARD
            Serial.println("Dir created");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("mkdir failed");
        #endif
    }
}

void removeDir(fs::FS &fs, const char * path){

    #if DEBUG_SDCARD
        Serial.printf("Removing Dir: %s\n", path);
    #endif
    if(fs.rmdir(path)){
        #if DEBUG_SDCARD
            Serial.println("Dir removed");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("rmdir failed");
        #endif
    }
}

String readFile(fs::FS &fs, const char * path){

    String str_read = "";

    #if DEBUG_SDCARD
        Serial.printf("Reading file: %s\n", path);
    #endif

    File file = fs.open(path);
    if(!file){
        #if DEBUG_SDCARD
            Serial.println("Failed to open file for reading");
        #endif
        return "";
    }
    #if DEBUG_SDCARD
        Serial.print("Read from file: ");
    #endif

    while(file.available()){        
        str_read.concat((char)file.read());        
    }
    file.close();

    return str_read;
}

void showInSerialFile(fs::FS &fs, const char * path){

    String str_read = "";
    
    Serial.printf("Reading file: %s\n", path);    

    File file = fs.open(path);
    if(!file){    
        Serial.println("Failed to open file for reading");    
        return;
    }
    Serial.print("Read from file: ");

    while(file.available()){        
        str_read.concat((char)file.read());        
    }
    file.close();

    Serial.println(str_read);
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    #if DEBUG_SDCARD
        Serial.printf("Writing file: %s\n", path);
    #endif

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        #if DEBUG_SDCARD
            Serial.println("Failed to open file for writing");
        #endif
        return;
    }
    if(file.print(message)){
        #if DEBUG_SDCARD
            Serial.println("File written");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("Write failed");
        #endif
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    #if DEBUG_SDCARD
        Serial.printf("Appending to file: %s\n", path);
    #endif

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        #if DEBUG_SDCARD
            Serial.println("Failed to open file for appending");
        #endif
        return;
    }
    if(file.print(message)){
        #if DEBUG_SDCARD
            Serial.println("Message appended");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("Append failed");
        #endif
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    #if DEBUG_SDCARD
        Serial.printf("Renaming file %s to %s\n", path1, path2);
    #endif
    if (fs.rename(path1, path2)) {
        #if DEBUG_SDCARD
            Serial.println("File renamed");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("Rename failed");
        #endif
    }
}

void deleteFile(fs::FS &fs, const char * path){
    #if DEBUG_SDCARD
        Serial.printf("Deleting file: %s\n", path);
    #endif
    if(fs.remove(path)){
        #if DEBUG_SDCARD
            Serial.println("File deleted");
        #endif
    } else {
        #if DEBUG_SDCARD
            Serial.println("Delete failed");
        #endif
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        #if DEBUG_SDCARD
            Serial.printf("%u bytes read for %u ms\n", flen, end);
        #endif
        file.close();
    } else {
        #if DEBUG_SDCARD
            Serial.println("Failed to open file for reading");
        #endif
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        #if DEBUG_SDCARD
            Serial.println("Failed to open file for writing");
        #endif
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    #if DEBUG_SDCARD
        Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    #endif
    file.close();
}

void initializeSDCARD(){

    if(!SD.begin()){
        #if DEBUG_SDCARD
            Serial.println("Card Mount Failed");
        #endif
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        #if DEBUG_SDCARD
            Serial.println("No SD card attached");
        #endif
        return;
    }

    #if DEBUG_SDCARD
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
    #endif

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    #if DEBUG_SDCARD
        Serial.printf("SD Card Size: %lluMB\n", cardSize);
        Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
        Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    #endif
}

#endif