// =========================================================================================================================================
//                                                 Rotating Display RDC
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


#ifndef ASYNCWEBSERVER_REGEX
#define ASYNCWEBSERVER_REGEX
#endif



/* The webInterface implementation has been inspired and uses code from 
 *  
 *  1. https://RandomNerdTutorials.com/esp32-async-web-server-espasyncwebserver-library/
 *  AsyncWebServer from Rui Santos
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  2. https://techtutorialsx.com/2018/07/23/esp32-arduino-http-server-template-processing-with-multiple-placeholders/amp/
 *  very useful tutorial on template processing with AsyncWebServer.
 *  
 *  However, in order to make the code more readable, the web pages are stored in sets of separate files, 
 *  .html, .css, and .js, respectively. These files are held in LittleFS file system. The ESPAsyncWebServer processor engine
 *  is NOT used.
 * 
 */

#include "my_ESP.h"
#include "webInterface.h"
#include "webinterface_data.h"
#include "FlashFS.h"
#include "RD_40.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>


// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

webInterface::webInterface() {
}

// =========================================================================================================================================
//                                                      begin Method:
// =========================================================================================================================================

void webInterface::begin(String ssid_) {
  _ssid = ssid_;

  apiKey = _apiKey_f.read_f();
  location = _location_f.read_f();
  country = _country_f.read_f();
  clockFacePath = _clockFacePath_f.read_f();
  logoPath = _logoPath_f.read_f();
  imagePath = _imagePath_f.read_f();
  
  _startServer();

}

void webInterface::update() {
  _server.handleClient();
}


// =========================================================================================================================================
//                                                        _startServer() and webSocket handlers
// =========================================================================================================================================

void webInterface::_serveFile(const char* filePath, const char* mimeType) {
  File file = SD.open(filePath, FILE_READ);
  if (!file) {
    _server.send(404, "text/plain", "File not found");
    return;
  }
  
  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
  }
  file.close();
  
  _server.send(200, mimeType, fileContent);
}

void webInterface::_startServer() {
  
  // ====================================================================================
  // index
  // ====================================================================================

  _server.on("/", [this]() {
    _serveFile("/html/index.html", "text/html");
  });

  _server.on("/index.css", [this]() {
    _serveFile("/html/css/index.css", "text/css");
  });

  _server.on("/index.js", [this]() {
    _serveFile("/html/scripts/index.js", "text/javascript");
  });

  _server.on("/favicon.ico", [this]() {
    _server.send(200, "text/plain", "OK");
  });

  _server.on("/getParam", [this]() {
    JsonDocument doc;
    doc["mode"] = clockMode;
    doc["brightness"] = brightness;

    Serial.print("Mode uploaded: ");
    Serial.println(clockMode);

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/configDone", [this]() {
    _serveFile("/html/index.html", "text/html");
  });

  _server.on("/updateradiobutton", [this] () {
    String button;
    if (_server.hasArg("button")) {       
      button = _server.arg("button");
  
      if (button=="option1") clockMode=0;
      if (button=="option2") clockMode=1;
      if (button=="option3") clockMode=2;
      if (button=="option4") clockMode=3;
      if (button=="option5") clockMode=4;
      if (button=="option6") clockMode=5;
      }
    else {
      clockMode=0;
    }
    _server.send(200, "text/plain", "OK");
  });

  _server.on("/slider", [this] () {
    if (_server.hasArg("value")) {
      _brightness_s = _server.arg("value");
      brightness = _brightness_s.toInt();
      Serial.println("Slider value = " + _brightness_s);
    } else {
      Serial.println("no slider value sent");
    }
    _server.send(200, "text/plain", "OK");
  });

  // ====================================================================================
  // configWeather
  // ====================================================================================

  _server.on("/configWeather", [this]() {
    _serveFile("/html/configWeather.html", "text/html");
  });

  _server.on("/configWeather.css", [this]() {
    _serveFile("/html/css/configWeather.css", "text/css");
  });

  _server.on("/configWeather.js", [this]() {
    _serveFile("/html/scripts/configWeather.js", "text/javascript");
  });

  _server.on("/getWeatherParam", [this]() {
    JsonDocument doc;
    doc["apiKey"] = _apiKey_f.read_f();
    doc["location"] = _location_f.read_f();
    doc["country"] = _country_f.read_f();
    doc["w_icon"] = _w_icon;
    doc["w_temp"] = _w_temp;
    doc["w_humi"] = _w_humi;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/getWeather", [this]() {
    JsonDocument doc;
    doc["w_icon"] = _w_icon;
    doc["w_temp"] = _w_temp;
    doc["w_humi"] = _w_humi;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/uploadWeatherParam", [this]() {
    if ((_server.hasArg("value1"))&&(_server.hasArg("value2"))&&(_server.hasArg("value3")))
    { 
      apiKey = _server.arg("value1");
      location = _server.arg("value2");
      country = _server.arg("value3");
 
      Serial.print("apiKey: ");
      Serial.println(apiKey);
      Serial.print("location: ");
      Serial.println(location);
      Serial.print("country: ");
      Serial.println(country);
      
      _apiKey_f.write_f(apiKey);
      _location_f.write_f(location);
      _country_f.write_f(country);
    }

    _server.send(200, "text/plain", "OK");
    updateWeather=true;
  });

  for (int i = 0; i < 9; i++) {
    String path = "/w_img" + String(i) + ".PNG";
    _server.on(path.c_str(), [this, i]() {
      String webImage = "/html/images/w_img" + String(i) + ".png";
      
      File file = SD.open(webImage, FILE_READ);
      String fileContent = "";
      while (file.available()) {
        fileContent += (char)file.read();
      }
      file.close();
      _server.send(200, "image/png", fileContent);
    });
  }


  // ====================================================================================
  // resetWifi
  // ====================================================================================

  _server.on("/resetWifi", [this]() {
    _serveFile("/html/resetWifi.html", "text/html");
  });

  _server.on("/resetWifi.css", [this]() {
    _serveFile("/html/css/resetWifi.css", "text/css");
  });

  _server.on("/resetWifi.js", [this]() {
    _serveFile("/html/scripts/resetWifi.js", "text/javascript");
  });

  _server.on("/reset", [this]() {
    _server.send(200, "text/plain", "OK");
    FlashFS* flashFs1 = new FlashFS("/variables/ssid.txt");
    flashFs1 -> delete_f();
    delete flashFs1;
    FlashFS* flashFs2 = new FlashFS("/variables/password.txt");
    flashFs2 -> delete_f();
    delete flashFs2;
  });

  _server.on("/getWifiParam", [this]() {
    JsonDocument doc;
    doc["ssid"] = _ssid_f.read_f();
    doc["password"] = _password_f.read_f();

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/uploadWifiParam", [this]() {
    if ((_server.hasArg("value1"))&&(_server.hasArg("value2"))){ 
      String _ssid = _server.arg("value1");
      String _password = _server.arg("value2");
      
      _ssid_f.write_f(_ssid);
      _password_f.write_f(_password);
    }
    _server.send(200, "text/plain", "OK");
  });


  // ====================================================================================
  // fileManager
  // ====================================================================================

  _server.on("/fileManager", [this]() {
    _serveFile("/html/fileManager.html", "text/html");
  });

  _server.on("/fileManager.css", [this]() {
    _serveFile("/html/css/fileManager.css", "text/css");
  });

  _server.on("/fileManager.js", [this]() {
    _serveFile("/html/scripts/fileManager.js", "text/javascript");
  });

  _server.on("/filelist", [this]() {
    _currentPath = _server.arg("path");
    String json;
    {
      String _currentPath_s = _currentPath;
      if ((_currentPath_s.length()>1) && (_currentPath_s[_currentPath_s.length() - 1] == '/')) {
        _currentPath_s = _currentPath_s.substring(0, _currentPath_s.length() - 1);
      }
      FlashFS flashFs(_currentPath_s);
      json = flashFs.listFilesInJson();
    }
    _server.send(200, "application/json", json);
  });

  _server.on("/deletefile", [this]() {
    String fileName = _server.arg("filename");
    Serial.print("webinterface: delete file: ");
    Serial.println(fileName);

    String fileName_s = fileName;
    if (fileName_s.length()!=0 && fileName_s[fileName_s.length() - 1] == '/') {
      fileName_s = fileName_s.substring(0, fileName_s.length() - 1);
    }

    FlashFS* flashFs = new FlashFS(fileName_s);
    flashFs -> delete_f();
    delete flashFs;

    String json;
    {
        FlashFS flashFs("/");
        json = flashFs.listFilesInJson();
    }
    _server.send(200, "application/json", json);
  });

  _server.on("/renamefile", [this]() {
    String oldFileName = _server.arg("oldfilename");
    String newFileName = _server.arg("newfilename");
    if (oldFileName.length()!=0 && oldFileName[oldFileName.length() - 1] == '/') {
      oldFileName = oldFileName.substring(0, oldFileName.length() - 1);
    }
    if (newFileName.length()!=0 && newFileName[newFileName.length() - 1] == '/') {
      newFileName = newFileName.substring(0, newFileName.length() - 1);
    }

    if (SD.exists(newFileName.c_str())) {
      // File with newFileName already exists, send an error response
      Serial.println("File with the new name already exists.");
      _server.send(400, "text/plain", "File with the new name already exists.");
      return;
    }

    Serial.print("rename file: ");
    Serial.print(oldFileName);
    Serial.print(" => ");
    Serial.println(newFileName);

    FlashFS* flashFs = new FlashFS(oldFileName);
    flashFs -> rename_f(newFileName);
    delete flashFs;

    String json;
    {
        FlashFS flashFs("/");
        json = flashFs.listFilesInJson();
    }
    _server.send(200, "application/json", json);
  });

  _server.on("/copyfile", [this]() {
    String sourcePath = _server.arg("source");
    String destPath = _server.arg("destination");
    int moveFlag = _server.arg("moveflag").toInt();
 
    if (sourcePath.indexOf('.') == -1) {             // if directory
      if (destPath.startsWith(sourcePath)) {
        return;
      }
      // determine destination path:
      // extract folder name from sourcePath
      String lastFolder = "";
      int secondLastSlashIndex = sourcePath.lastIndexOf("/", sourcePath.length()-2);
      if (secondLastSlashIndex >= 0) {
        lastFolder = sourcePath.substring(secondLastSlashIndex + 1, sourcePath.length()-1);
      }
      destPath += lastFolder;
      destPath += "/";
      sourcePath = sourcePath.substring(0, sourcePath.length()-1);
    }
    else {
      sourcePath = sourcePath.substring(0, sourcePath.length());
    }
    destPath = destPath.substring(0, destPath.length()-1);

    Serial.println("/copyfile from "+sourcePath+" to "+destPath);

    FlashFS* flashFs = new FlashFS(sourcePath);
    flashFs -> copy_f(destPath);
    delete flashFs;

    if (moveFlag != 0) {
      FlashFS* flashFs = new FlashFS(sourcePath);
      flashFs -> delete_f();
      delete flashFs;
    }
    String json;
    {
        FlashFS flashFs("/");
        json = flashFs.listFilesInJson();
    }
    _server.send(200, "application/json", json);
  });

  _server.on("/mkdir", [this]() {
    String newDir = _server.arg("filename");
    if (newDir.length()!=0 && newDir[newDir.length() - 1] == '/') {
      newDir = newDir.substring(0, newDir.length() - 1);
    }
    FlashFS* flashFs = new FlashFS(newDir);
    delete flashFs;

    String json;
    {
        FlashFS flashFs("/");
        json = flashFs.listFilesInJson();
    }
    _server.send(200, "application/json", json);
  });

  _server.on("/uploadfile", HTTP_POST, [this](){ _server.send(200);}, [this]() {_handleFileUpload();});

  _server.on("/downloadfile", [this]() {
    if (_server.hasArg("filename")) {
      String filename = _server.arg("filename");
      String contentType = "application/octet-stream";
      if (filename.endsWith(".txt")) {
        contentType = "text/plain";
      } else if (filename.endsWith(".js")) {
        contentType = "application/javascript";
      } else if (filename.endsWith(".css")) {
        contentType = "text/css";
      } else if (filename.endsWith(".html")) {
        contentType = "text/html";
      } else if (filename.endsWith(".gif")) {
        contentType = "image/gif";
      } else if (filename.endsWith(".jpeg") || filename.endsWith(".jpg")) {
        contentType = "image/jpeg";
      } else if (filename.endsWith(".tiff") || filename.endsWith(".tif")) {
        contentType = "image/tiff";
      } else if (filename.endsWith(".png")) {
        contentType = "image/png";
      } else if (filename.endsWith(".webp")) {
        contentType = "image/webp";
      } else if (filename.endsWith(".bmp")) {
        contentType = "image/bmp";
      }

      File download = SD.open(filename);
      if (download) {
        _server.sendHeader("Content-Type", contentType);
        _server.sendHeader("Content-Disposition", "attachment; filename="+filename);
        _server.sendHeader("Connection", "close");
        _server.streamFile(download, "application/octet-stream");
        download.close();
        _server.send(200, "text/html", "download sucessfully completed");
      } else {
        _server.send(404, "text/plain", "not found");
      }
    }  
  });

  _server.on("/fileSize", [this]() {
    if (_server.hasArg("filename")) {
      String filename = _server.arg("filename");
      if (SD.exists(filename)) {
        File file = SD.open(filename, FILE_READ);
        String fileSize = String(file.size());
        _server.send(200, "application/json", "{\"size\":" + fileSize + "}");
        file.close();
      } else {
        _server.send(404);
      }
    }
  });

 _server.on("/diskspace", [this]() {
    String totalBytes = String(SD.totalBytes());
    String usedBytes = String(SD.usedBytes());
    String response = "{\"totalBytes\": " + totalBytes + ", \"usedBytes\": " + usedBytes + "}"; // JSON-Response erstellen
    _server.send(200, "application/json", response); // Response zurücksenden
  });

  // ====================================================================================
  // RDC Image Manager
  // ====================================================================================

  _server.on("/rdcImageManager", [this]() {
    _serveFile("/html/rdcImageManager.html", "text/html");
  });

  _server.on("/rdcImageManager.css", [this]() {
    _serveFile("/html/css/rdcImageManager.css", "text/css");
  });

  _server.on("/rdcImageManager.js", [this]() {
    _serveFile("/html/scripts/rdcImageManager.js", "text/javascript");
  });

  _server.on("/omggif.js", [this]() {
    _serveFile("/html/omggif/omggif.js", "text/javascript");
  });

  _server.on("/selectclockface", [this]() {
    if (_server.hasArg("filename")) {
      clockFacePath = _server.arg("filename");
      _clockFacePath_f.write_f(clockFacePath);
      _server.send(200, "text/plain", "File selection saved successfully.");
    } else {
      _server.send(400, "text/plain", "Invalid request.");
    }
  });

  _server.on("/selectlogo", [this]() {
    if (_server.hasArg("filename")) {
      logoPath = _server.arg("filename");
      _logoPath_f.write_f(logoPath);

      Serial.print("selected logo: ");
      Serial.println(logoPath);
      _server.send(200, "text/plain", "File selection saved successfully.");
    } else {
      _server.send(400, "text/plain", "Invalid request.");
    }
  });

  _server.on("/selectimage", [this]() {
    if (_server.hasArg("filename")) {
      imagePath = _server.arg("filename");
      _imagePath_f.write_f(imagePath);

      Serial.print("selected image: ");
      Serial.println(imagePath);
      _server.send(200, "text/plain", "File selection saved successfully.");
    } else {
      _server.send(400, "text/plain", "Invalid request.");
    }
  });

  _server.on("/getRDCNames", [this]() {
    JsonDocument doc;
    doc["clockface"] = _clockFacePath_f.read_f();
    doc["logo"] = _logoPath_f.read_f();
    doc["image"] = _imagePath_f.read_f();

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });


  _server.on("/downloadfilepart1", [this]() {
    if (_server.hasArg("filename")) {
      String filename = _server.arg("filename");
      String contentType = "application/octet-stream";
      Serial.println("/downloadfilepart1 filename: "+filename);

      FlashFS* sourceFile = new FlashFS(filename);
      String tempFileName = "/temp.rdc";
      sourceFile -> copyFirstImage(tempFileName);
      delete sourceFile;

      String fileContent = "";
      File file = SD.open(tempFileName, FILE_READ);
      while (file.available()) {
        fileContent += (char)file.read();
      }
      file.close();
      _server.send(200, contentType, fileContent);
    }
  });


  // ====================================================================================
  // time zone
  // ====================================================================================


  _server.on("/timeZone", [this]() {
    _serveFile("/html/timeZone.html", "text/html");
  });

  _server.on("/timeZone.css", [this]() {
    _serveFile("/html/css/timeZone.css", "text/css");
  });

  _server.on("/timeZone.js", [this]() {
    _serveFile("/html/scripts/timeZone.js", "text/javascript");
  });

  _server.on("/timeZoneData", [this]() {
    
    String part;
    if (_server.hasArg("part")) {
      part = _server.arg("part");
    }

    // Create a JSON document
    JsonDocument doc;

    // Create an array in the JSON document
    JsonArray timeZoneArray = doc.to<JsonArray>();

    int part_i = part.toInt()-1;
    // Iterate through the timeZones array and add objects to the JSON array
    for (int i = part_i*20; i < (20+part_i*17); i++) {
      char location[50];
      char timeZone[50];
      char timeDifference[50];

      // Read data from PROGMEM and copy it to SRAM variables
      for (byte k = 0; k < 50; k++) {
          location[k] = pgm_read_byte_near(timeZones[0][i] + k);
          timeZone[k] = pgm_read_byte_near(timeZones[1][i] + k);
          timeDifference[k] = pgm_read_byte_near(timeZones[2][i] + k);
      }

      char entry[150];
      strcpy(entry, location);
      strcat(entry, ": ");
      strcat(entry, timeZone);
      strcat(entry, " (");
      strcat(entry, timeDifference);
      strcat(entry, ")");

    JsonObject timeZoneObj = timeZoneArray.add<JsonObject>();
      timeZoneObj["entry"] = entry;
    }
    // Serialize the JSON document to a string
    String jsonString;
    serializeJson(doc, jsonString);

    // Set the Content-Type header to application/json
    _server.send(200, "application/json", jsonString);
  });

  _server.on("/timeZoneUpdate", [this] () {
    if (_server.hasArg("value")) {
      String newTimeZone = _server.arg("value");
      int newTimeZone_i = newTimeZone.toInt();
      Serial.println("new time zone = " + newTimeZone);

      char timeZone[50];

      // Read data from PROGMEM and copy it to SRAM variables
      for (byte k = 0; k < 50; k++) {
          timeZone[k] = pgm_read_byte_near(timeZones[3][newTimeZone_i - 1] + k);
      }

      String timeZone_s(timeZone);
      _timeZone_f.write_f(timeZone_s);

      my_ESP ESP_temp;
      ESP_temp.setMyTime(); 

      Serial.println(timeZone_s);

    } else {
      Serial.println("no time zone sent");
    }
    _server.send(200, "text/plain", "OK");
  });

  _server.on("/server-time", [this]() {
      // Erzeuge das JSON-Response
      String jsonResponse = "{";
      jsonResponse += "\"hour\": " + String(myESP.Hour) + ",";
      jsonResponse += "\"minute\": " + String(myESP.Min) + ",";
      jsonResponse += "\"second\": " + String(myESP.Sec);
      jsonResponse += "}";

      // Sende das JSON zurück an den Client
      _server.send(200, "application/json", jsonResponse);
  });

  // ====================================================================================
  // Start server
  // ====================================================================================

  _server.begin();  
  Serial.println("server started");

}

void webInterface::updateWI(int w_icon, String w_temp, String w_humi) {
  _w_icon = w_icon;
  _w_temp = w_temp;
  _w_humi = w_humi;
}

String webInterface::_currentPath = "";

/////////////////////////////////////////////////////////////////////////////////////////////////
// File handling example: https://github.com/G6EJD/ESP32-8266-File-Upload/blob/master/ESP_File_Download_Upload.ino
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void webInterface::_handleFileUpload(){ 
  HTTPUpload& uploadfile = _server.upload();
  
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename_u, myPath_u;
      
    filename_u = uploadfile.filename;
    myPath_u = _currentPath + filename_u;
    Serial.print("Upload File Name: "); Serial.println(myPath_u);

    SD.remove(myPath_u);                         // Remove a previous version, otherwise data is appended the file again
    _UploadFile = SD.open(myPath_u, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
    if (_UploadFile!=0) Serial.println ("myPath "+myPath_u+" opened successfully");
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(_UploadFile) _UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(_UploadFile)          // If the file was successfully created
    {                                    
      _UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      _server.send(200);
    } 
    else
    {
      _server.send(404, "text/plain", "Not Found");    
    }
  }
};