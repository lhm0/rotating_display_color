// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "my_ESP.h"
#include "RD_40.h"
#include "FlashFS.h"

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FlashFS.h>

#define TIME_ZONE "CET-1CEST,M3.5.0/02,M10.5.0/03" // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv    
                                                   // this is only the default (my home ;-)       
#define NTP "pool.ntp.org"

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

my_ESP::my_ESP() {}

// =========================================================================================================================================
//                                                      begin Method
// =========================================================================================================================================

void my_ESP::begin() {

  // initiate Wifi
  _iniWifi();

  // set time
  setMyTime();
}

// =========================================================================================================================================
//                                                      Wifi Methods
// =========================================================================================================================================

void my_ESP::_iniWifi() {
  /*
    read SSID and password, if available
  */
  Serial.println ("start initialization....");

  _ssid_f.begin();
  Serial.println("nun folgt .read_f()");
  _ssid = _ssid_f.read_f();
  Serial.println(".read_f() abgeschlossen");
  _password = _password_f.read_f();
  Serial.println(_ssid);
  Serial.println(_password);

  /*
    try to connect to WiFi, if ssid and password are valid
  */
  if ((_ssid!="")|(_password!="")) { 
    WiFi.begin(_ssid, _password);

    Serial.print("connecting to WiFi ...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(1000);
      Serial.print(".");
      attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
      ssid = _ssid;
      ipAddress = WiFi.localIP().toString();
      Serial.println("WiFi connected");
  } else {
      Serial.println("Wifi connection failed...");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {

    WiFi.mode(WIFI_OFF);
    delay(1000);
    WiFi.mode(WIFI_AP);                     //
    IPAddress apIP(192, 168, 4, 1);         //
    IPAddress subnet(255,255,255,0);        //
    WiFi.softAPConfig(apIP, apIP, subnet);  //

    ssid = "RD56c";
    WiFi.softAP(ssid);                      //

    ipAddress = WiFi.softAPIP().toString();

    Serial.println("Access Point started.");
    Serial.print("IP-Adresse: ");
    Serial.println(ipAddress);  
  }

}

// =========================================================================================================================================
//                                                      Time Methods
//                                  https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
// =========================================================================================================================================

void my_ESP::setMyTime() {
  _timeZone = _timeZone_f.read_f();
  if (_timeZone == "") _timeZone = "CET-1CEST,M3.5.0/02,M10.5.0/03";
  char _timeZone_c[50];
  strcpy(_timeZone_c, _timeZone.c_str());

  configTime(0, 0, NTP);  // 0, 0 because we will use TZ in the next line
  setenv("TZ", _timeZone_c, 1);            // Set environment variable with your time zone
  tzset();
  
}

void my_ESP::getMyTime() {
  time(&_now);                 // read the current time
  localtime_r(&_now, &tm1);    // update the structure tm1 with the current time

  Hour = tm1.tm_hour;
  Min = tm1.tm_min;
  Sec = tm1.tm_sec;
  Mday = tm1.tm_mday;
  Mon = tm1.tm_mon + 1;
  Year = tm1.tm_year + 1900;
  Wday = tm1.tm_wday;
}
