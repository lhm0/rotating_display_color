// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


#include "FlashFS.h"
#include <Arduino.h>
#include "FS.h"
#include <SD.h>
#include <SPI.h>

#define HSPI_MOSI 5
#define HSPI_SCLK 7
#define HSPI_MISO 6
#define HSPI_CS 8 

// Constructor

FlashFS::FlashFS(String path) {
  this->_path = path;
  if (_isDirectory()) {
      _mkdir(_path);              // creat path, if it does not exist yet
    }
}
// Initialisierungsroutine

bool FlashFS::begin() {
  _SPI2.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
  pinMode(HSPI_CS, OUTPUT);
  bool initok = false;

  initok = SD.begin(HSPI_CS, _SPI2);       
  if (!(initok))  {
    Serial.println("Card Mount Failed"); // check, if card o.k.
  }
  else {
      uint8_t cardType = SD.cardType();

      if(cardType == CARD_NONE){
          Serial.println("No SD card attached");
          return false;
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

  }
  return initok;
}

// Routine zum Lesen einer Datei
String FlashFS::read_f() {
  File file = SD.open(_path.c_str(), "r", false);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  String text = file.readString();
  file.close();
  return text;
}

// read the data into bitmap array. lower left hand corner is x, y
bool FlashFS::read_f(uint8_t (*bitmap)[165], int x, int y, int frame) {
  File file = SD.open(_path.c_str());
  uint8_t buffer[165];
  bool success = true;
  unsigned int xd, yd, noOfFrames;

  if (!file) {
    Serial.println("Failed to open file for reading");
    success = false;
    return success;
  } 

  if (file.read((uint8_t*)&xd, sizeof(xd)) != sizeof(xd)) {
    Serial.println("Failed to read xd from file");
    success = false;
    return success;
  }

  if (file.read((uint8_t*)&yd, sizeof(yd)) != sizeof(yd)) {
    Serial.println("Failed to read yd from file");
    success = false;
    return success;
  }

  if (file.read((uint8_t*)&noOfFrames, sizeof(noOfFrames)) != sizeof(noOfFrames)) {
    Serial.println("Failed to read noOfFrames from file");
    success = false;
    return success;
  }

  int ymax = yd*3/2;
  if ((yd*3%2)!=0) ymax++;

  unsigned long offset = 12 + frame * xd * ymax;
  file.seek(offset);   //go to start position of frame
//  Serial.printf("Frame: %d  Offset: %d\n", frame, offset);

  y = (y*3/2)/2*2;            // make y an even number
  if ((y+ymax)>=165) ymax = 165 - y-1;


  for (int xs=x; ((xs<xd+x) && (xs<110)); xs++) {
    int bytesRead = file.read(buffer, (yd*3/2));       // read bytes of one column
    memcpy(&bitmap[xs][y], buffer, ymax);
  }
      
  file.close();
  return success;
}

// Routine zum Schreiben einer Datei
void FlashFS::write_f(String text) {

  File file = SD.open(_path.c_str(), FILE_WRITE);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(text);
  file.close();
}

void FlashFS::write_f(uint8_t* data, size_t len) {
  File file = SD.open(_path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.write(data, len);
  file.flush();
  file.close();
}

File* FlashFS::open_f(String mode) {
  if (_isDirectory()) {
    return nullptr; // return null pointer to indicate error
  }
  _file = new File(SD.open(_path.c_str(), mode.c_str()));
  return _file;
}

bool FlashFS::close_f() {
  if (_file) {
    _file->close();
    delete _file;
    _file = nullptr;
    return true; // successful close
  }
  return false; // file not open
}

String FlashFS::listFilesInJson() {
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  File root = SD.open(_path.c_str());
  if(!root){
    Serial.println("Failed to open directory");
    return "";
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return "";
  }

  File file = root.openNextFile();
  while(file){
    String fileName = file.name();
    if ((fileName[0] != '.')&&(fileName !="System Volume Information")) {
      array.add(fileName);
    }
    file = root.openNextFile();
  }

  String jsonString;
  serializeJson(doc, jsonString);

  return jsonString;
}

bool FlashFS::rename_f(String newPath) {
  bool success; 
  
  if (_isDirectory()) {                                                       //// !!!! ////
   success = SD.rename(_path.c_str(), newPath.c_str());
  } else {
    success = SD.rename(_path.c_str(), newPath.c_str());
  }
  
  if (success) {
    _path = newPath;
  }
  return success;
}

bool FlashFS::delete_f() {
  if (_isDirectory()) {
    // remove all files and subdirectories recursively, before deleting the directory
    File root = SD.open(_path.c_str());
    if (!root) {
      return false;
    }
    
    File file = root.openNextFile();
    while(file){
      String fileName = file.name();
      String filePath = _path + "/" + fileName;
      if (file.isDirectory()) {
        // delete subdirectory
        FlashFS* subDir = new FlashFS(filePath);
        subDir -> delete_f();
        delete subDir;
      } else {
        if (!SD.remove(filePath.c_str())) {
          return false;
        }
      }
      file = root.openNextFile();    
    }
    // now directory is empty. remove the directory
    return SD.rmdir(_path.c_str());
  } else {
    // delete a single file
    return SD.remove(_path.c_str());
  }
}

bool FlashFS::copy_f(String destPath) {
  String sourcePath = _path;
  if (_isDirectory()) {
    // create destination directory if it doesn't exist
    _mkdir(destPath);
    // copy all files and subdirectories recursively
    File root = SD.open(_path.c_str());
    File file = root.openNextFile();
    while(file) {
      String sourceFileName = file.name();
      String sourceFilePath = sourcePath + "/" + sourceFileName;

      if (file.isDirectory()) {
        String subDirDestPath = destPath + "/" + sourceFileName;
        FlashFS* subDir = new FlashFS(sourceFilePath);
        subDir -> copy_f(subDirDestPath);
        delete subDir;
      } else {
        String destFilePath = destPath + "/" + sourceFileName;
        File* srcFile = new File(SD.open(sourceFilePath.c_str(), "r"));
        File* destFile = new File(SD.open(destFilePath.c_str(), "w"));
        if (!srcFile || !destFile) {
          return false; // copy failed
        }
        while (srcFile->available()) {
          destFile->write(srcFile->read());
          destFile->flush();
        }
        srcFile->close();
        destFile->close();
        delete srcFile;
        delete destFile;
      }

      file = root.openNextFile();    
    }
    return true;
  } else {
    // determine destination path:
    // extract file name from sourcePath
    String fileName = "";
    int lastSlashIndex = sourcePath.lastIndexOf("/");
    if (lastSlashIndex >= 0) {
      fileName = sourcePath.substring(lastSlashIndex + 1);
    }
    destPath = destPath + "/" + fileName;

    // copy file to destination
    File* srcFile = new File(SD.open(sourcePath.c_str(), "r"));
    File* destFile = new File(SD.open(destPath.c_str(), "w"));
    if (!srcFile || !destFile) {
      return false; // copy failed
    }
    while (srcFile->available()) {
      destFile->write(srcFile->read());
      destFile->flush();
    }
    srcFile->close();
    destFile->close();
    delete srcFile;
    delete destFile;
    return true;
  }
}

bool FlashFS::move_f(String destPath) {
  if (copy_f(destPath)) {
    delete_f();
    _path = destPath;
    return true;
  } else {
    return false; // move failed
  }
}

bool FlashFS::_isDirectory() {
    return _path.indexOf('.') == -1;
}

bool FlashFS::_mkdir(String path) {
  if (path.endsWith("/")) {                   // check if path ends with "/"
    path.remove(path.length()-1);             // remove the last character
  }
  if (!SD.exists(path.c_str())) {             // check if directory already exists
    bool success = SD.mkdir(path.c_str());    // make directory, if it does not exist, yet
      if (!success) {
        Serial.println("Failed to create directory");
      }
    return success;
  } else {
    return true;
  }
}

bool FlashFS::exists() {
  Serial.println("exists(): "+_path);
  return !SD.exists(_path);
}

// read RDC file, generate new destFile, which contains first image of the source
bool FlashFS::copyFirstImage(String destFileName) {
  byte* buffer = new byte[18165];
  int bytesRead;
  bool success = true;
  unsigned int xd, yd, noOfFrames;

  File file = SD.open(_path.c_str());
  
  if (!file) {
    Serial.println("Failed to open sourceFile for reading");
    success = false;
    return success;
  } 

  if (file.read((uint8_t*)&xd, sizeof(xd)) != sizeof(xd)) {
    Serial.println("Failed to read xd from file");
    success = false;
    return success;
  }

  if (file.read((uint8_t*)&yd, sizeof(yd)) != sizeof(yd)) {
    Serial.println("Failed to read yd from file");
    success = false;
    return success;
  }

  if (file.read((uint8_t*)&noOfFrames, sizeof(noOfFrames)) != sizeof(noOfFrames)) {
    Serial.println("Failed to read noOfFrames from file");
    success = false;
    return success;
  }

  int ymax = yd*3/2;
  if ((yd*3%2)!=0) ymax++;

  int bytesFirstImage = 12 + xd * ymax;
  file.seek(0);   //go to beginning of file
  bytesRead = file.read(buffer, bytesFirstImage);       // read first image
      
  file.close();

  File destFile = SD.open(destFileName.c_str(), FILE_WRITE);
  if (!destFile) {
    Serial.println("Failed to open destFile for writing");
    success = false;
    return success;
  } 
  destFile.write(buffer, bytesRead);
  destFile.close();

  delete[] buffer;

  return success;
}  