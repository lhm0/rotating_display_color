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
    Serial.println(_path + "Failed to open file for reading");
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
  if (SD.exists(_path.c_str())) SD.remove(_path);   // replace existing file with new one


  File file = SD.open(_path.c_str(), FILE_WRITE);

  if (!file) {
    Serial.println(_path + "Failed to open file for writing");
    return;
  }
  file.print(text);
  file.flush();
  file.close();
}

void FlashFS::write_f(uint8_t* data, size_t len) {
  File file = SD.open(_path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println(_path + "Failed to open file for writing");
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

/////////////////////////////////////////////////////////
// the following functions manipulate files and directories.
// path formate:
// - folders end with "/"
// - the root folder is "/"
// - files have an extension ".xxx"
// 
// examples
// /variables/ssid.txt
// /variables/
// 
// in order to use the functions, a source object needs to be created:
// FlashFS mysource{"/variables/ssid.txt"}
// then, the function with its target path needs to be called:
// mysource.rename_f("/variables/new_ssid.txt")
// mysource.move_f("/ssid.txt")
// mysource.copy_f("/ssid.txt")
//
// This syntax also works for folders:
// FlashFS mysource{"/variables/test/"}
// mysource.rename_f("/variables/new_test/")
// mysource.move_f("/test/")
// mysource.copy_f("/test/")
/////////////////////////////////////////////////////////

bool FlashFS::_isDirectory() {
  return _path.indexOf('.') == -1;
}

bool FlashFS::_mkdir(String path) {
  Serial.println("_mkdir - path: " + path);

  if (path=="/") return true;
  if (path.endsWith("/")) {                   // check if path ends with "/"
    path.remove(path.length()-1);             // remove the last character
  }
  if (!SD.exists(path.c_str())) {             // check if directory already exists
    bool success = SD.mkdir(path.c_str());    // make directory, if it does not exist, yet
      if (!success) {
        Serial.println(path + ": Failed to create directory");
      }
    return success;
  } else {
    return true;
  }
}


String FlashFS::listFilesInJson() {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  File root = SD.open(_path.c_str());
  if(!root){
    Serial.println(_path+": Error opening directory");
return "";
  }
  if(!root.isDirectory()){
    Serial.println(_path+": Not a directory");
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
  Serial.println("rename_f - _path: " + _path);
  Serial.println("rename_f - newPath: " + newPath);

  bool success; 
  
    success = SD.rename(_path.c_str(), newPath.c_str());
  
  if (success) {
    _path = newPath;
  }
  return success;
}

bool FlashFS::delete_f() {
  Serial.println("delete_f - _path: " + _path);

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
      Serial.println("next file to delete, filePath: "+filePath);
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
  Serial.println("copy_f - _path: " + _path);
  Serial.println("copy_f - destPath: " + destPath);

  String sourcePath = _path;
  if (_isDirectory()) {
    // create destination directory if it doesn't exist
    _mkdir(destPath);
    // copy all files and subdirectories recursively
    File root = SD.open(_path.c_str());
    File file = root.openNextFile();
    while(file) {
      String sourceFileName = file.name();
      if (!sourceFileName.startsWith(".")) {   //ignore hidden files and folders   
        String sourceFilePath = sourcePath + "/" + sourceFileName;
        Serial.println ("next file to copy, sourceFilePath: "+sourceFilePath);

        if (file.isDirectory()) {
          String subDirDestPath = destPath + "/" + sourceFileName;
          FlashFS* subDir = new FlashFS(sourceFilePath);
         subDir -> copy_f(subDirDestPath);
          delete subDir;
        } else {
          String destFilePath = destPath + "/" + sourceFileName;
          Serial.println("next file to copy, destFilePath: "+destFilePath);
          File* srcFile = new File(SD.open(sourceFilePath.c_str(), "r"));
          File* destFile = new File(SD.open(destFilePath.c_str(), "w"));
          if (!srcFile || !destFile) {
            return false; // copy failed
         }
          while (srcFile->available()) {
            destFile->write(srcFile->read());
          }
          destFile->flush();

          srcFile->close();
          destFile->close();
          delete srcFile;
          delete destFile;
        }
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

    Serial.println("copy_f: sourcePath = "+sourcePath+" destPath = "+destPath);


    // copy file to destination
    if (!SD.exists(destPath)) {
      File srcFile = SD.open(sourcePath.c_str(), "r");
      File destFile = SD.open(destPath.c_str(), "w");
      if (!srcFile || !destFile) {
        Serial.println("error opening the file.");

        return false; // copy failed
      }
      size_t n;  
      uint8_t buf[64];
      while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
        destFile.write(buf, n);
      }
      srcFile.close();
      destFile.close();
    }
    else {
      Serial.println("copy_f: \"" + destPath + "\" already exists");
    }
    return true;
  }
}

bool FlashFS::move_f(String destPath) {
  Serial.println("move_f - _path: " + _path);
  Serial.println("move_f - destPath: " + destPath);

  if (copy_f(destPath)) {
    delete_f();
    _path = destPath;
    return true;
  } else {
    return false; // move failed
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

  SD.remove(destFileName.c_str());  
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