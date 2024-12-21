// =========================================================================================================================================
//                                                 Rotating Display RD56c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================



// ******************************************************************************************************************************************
//
//          This class provides the user interface for controlling the rotating display
//
//          The initialization method "webInterface::begin(my_ESP* myESP) uses the WebServer object "server" from myESP.
//          The web pages are stored on the SD card. 
//
// ******************************************************************************************************************************************


#ifndef webInterface_H
#define webInterface_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>

#include "my_ESP.h"
#include "FlashFS.h"

extern my_ESP myESP;

class webInterface {
  private:
    WebServer _server{80};                 // server object
    void _serveFile(const char* filePath, const char* mimeType);   // helper Funktion to send file 

    static FlashFS _dummy;
    FlashFS _apiKey_f{"/variables/apiKey.txt"};
    FlashFS _location_f{"/variables/location.txt"};
    FlashFS _country_f{"/variables/country.txt"};
    FlashFS _logoPath_f{"/variables/logoPath.txt"};
    FlashFS _clockFacePath_f{"/variables/clockFacePath.txt"};
    FlashFS _imagePath_f{"/variables/imagePath.txt"};
    FlashFS _ssid_f{"/variables/ssid.txt"};
    FlashFS _password_f{"/variables/password.txt"};
    FlashFS _timeZone_f{"/variables/timeZone.txt"};
    static String _currentPath;

    static File _activeUpload;
    static String _activeUploadFileName;

    int _w_icon=0;
    String _w_temp="99";
    String _w_humi="99";

    String _brightness_s;
    String _ssid; 

    static webInterface* _instance;

    void _startServer();
    void _handleFileUpload();
    File _UploadFile;

  public:
    webInterface();                                             // constructor

    void begin(String ssid_);                                  // starts the web server
    void update();
    int clockMode=0;
    int brightness=50;
    bool pauseImageUpdate = false;
    
    bool updateWeather=true;                                    // if true, main loop updates weather data
    String apiKey="";                                           // apiKey for openweathermap.org
    String location="";                                         // location for openweathermap.org
    String country="";                                          // country for openweathermap.org
    String logoPath="";                                         // path of logo file
    String clockFacePath="";                                    // path of clock face
    String imagePath="";                                        // path of image
    void updateWI(int w_icon, String w_temp, String w_humi);    // update web interface with weather data

};

#endif    
