// =========================================================================================================================================
//                                                 Rotating Display RD56c
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


#include <Arduino.h>

#include "my_ESP.h"
#include "RD_40.h"
#include "RD_40trafo.h"
#include "my_BMP.h"
#include "my_BMPtemplates.h"
#include "FlashFS.h"
#include "webInterface.h"

#include <WiFi.h>
#include <time.h>

// =========================================================================================================================================
// 
//                                                        Rotating Display
//
// The program uses OOP in order to structure data and functions. These are the classes:
//
// 1. my_ESP
//    description:    the class manages the resources of the ESP microcontroller. 
//    key methods:    begin()                                         configures Wifi, i2c interface, sets time 
//                    getMyTime()                                     occupies the time and date attributes (see below) 
//                    myPassword()                                    returns Wifi Password which is being used
//                    mySSID()                                        returns Wifi SSID
//    key attributes: int Sec, int Min, int Hour, ...
//
// 2. RD_40
//    description:    the class provides the central rotating display object. It manages the data of the displayed image (one bit per LED), 
//                    etc. 
//                    The class has methods for displaying bitmaps and for transferring data to the display controller. 
//    key methods:    begin()                                         initialize display mode and data transfer to the display controller
//                    upload()                                        uploads display data to the display controller
//                    displayBMP(unsigned char bmp[][14])             displays bitmap
//    key attributes: clockMode, brightness
//
// 3. my_BMP
//    description:    this class generates bitmaps for the display. It provides methods for generating clock and weather screens, 
//                    as well as printing text on bitmaps. The bitmap shall be handed over to the RD40 object. 
//    key methods:    generateBMP(int mode, tm* tm, String* ssid)     generates bitmap according to mode, time, ssid
//    key attributes: bitmap[][]                                      this is a 110x110 pixel bitmap
//
// 4. webInterface
//    description:    the class manages the web user interface. It uses the AsyncWebServer library and runs in the background without any need for
//                    attention. The class provides certain attributes, which are set by the web interface:
//    key methods:    begin(String* ssid_)                            starts AsyncWebServer. ssid is displayed (not changed) in web interface
//    key attributes: int clockMode;                                  mode 0 = IP-address, mode 1 = analog clock, mode 2 = digital clock, mode 3 = logo clock, mode 4 = weather clock
//                    int brightness;                                 brightness value in %
//                    char apiKey[50]="";                             apiKey for openweathermap.org
//                    char location[50]="";                           location for openweathermap.org
//                    char country[20]="";                            country for openweathermap.org
//
//
// =========================================================================================================================================


my_ESP myESP;                                                // create instance of my_ESP
webInterface wi56;                                           // create instance of webInterface
RD_40 RD40;                                                  // create instance of RD_40
my_BMP myBMP;                                                // create instance of my_BMP

long lastWeather;                                            // remember time of last weather updte
int lastUpdate=0;                                            // remember the second of last display update
int prevSec=0;
int prevClockMode=4;                                         // remember previous clockMode

int right = 0;

// =========================================================================================================================

void setup(){

  // Serial port for debugging purposes
  Serial.begin(115200);
    delay(1000);

  myESP.begin();                            // initiates Wifi, time
  String myssid = myESP.ssid;               // withdraw SSID
  String myIpAddress = myESP.ipAddress;
  wi56.begin(myssid);                       // start web server

  myESP.getMyTime();
  Serial.printf("Zeit: %d:%d:%d\n", myESP.Hour, myESP.Min, myESP.Sec);

  RD40.begin();                             // initiate RD40 object
}

void loop() {
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//

  if ( (((millis()-lastWeather)>60000)&&(wi56.clockMode==4)) || wi56.updateWeather) {     // update weather every 60 seconds OR if update requested with update_weather
    lastWeather=millis();
    Serial.println("request weather data.....");
    myBMP.getWeather(wi56.apiKey, wi56.location);
    wi56.updateWI(myBMP.w_icon, myBMP.w_temp, myBMP.w_humi);                               //send data via WebSocket to webpage

    wi56.updateWeather=false;
  }

  myESP.getMyTime();
  if (myESP.Sec!=prevSec) {
    prevSec = myESP.Sec;
    Serial.printf("%d:%d:%d   %d.%d.%d\n", myESP.Hour, myESP.Min, myESP.Sec, myESP.Mday, myESP.Mon, myESP.Year );
  }

long stamp = millis();                                           // current time
  if ((stamp-lastUpdate)>100) {
    lastUpdate = stamp;
    tm* mytm = &myESP.tm1; 

    myBMP.generateBMP(wi56.clockMode, mytm, myESP.ssid.c_str(), myESP.ipAddress.c_str());  // generate bitmap

    unsigned char (*mybitmap)[165] = myBMP.bitmap;                     // pointer at bitmap array

    RD40.displayBMP(mybitmap);                                        // update display data according to bitmap

    int mybrightness = wi56.brightness;                               // wi56 holds the brightness setting of user interface
    RD40.upload(mybrightness);                                          // 
  }

  wi56.update();
}
