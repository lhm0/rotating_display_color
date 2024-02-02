// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


// ******************************************************************************************************************************************
//
//          this class generates a bitmap for the display. It provides methods for generating clock and weather screens,
//          as well as printing text on the bitmap. The bitmap shall be handed over to the RD40 object. 
//          
// ******************************************************************************************************************************************


#ifndef my_BMP_H
#define my_BMP_H

#include <Arduino.h>
#include <WiFi.h>
#include "FlashFS.h"

class my_BMP {
  private:
    struct ImageParameters {
      String fileName;
      int xdim;
      int ydim;
      int numberOfFrames;
      int frameNumber;
    };

    ImageParameters _bmpImage, _bmpWf, _bmpLogo, _bmpIP, _bmpWeather;

    void _initImageParameters(ImageParameters& imageParameters, String imagePath); 
                                                        // reads the image parameters from the file and saves results to the structure

    void _generateIP();                                 // generates bitmap, which shows the IP address
    void _generateAnalog();                             // generates bitmap with analog clock
    void _generateDigital();                            // generates bitmap with digital clock
    void _generateLogoClock();                          // generates bitmap with logo clock
    void _generateWeather();                            // generates bitmap with weather information
    void _generateImage();                              // generates bitmap with image
    
    void _loadLogo();
    void _loadWatchFace();
    void _clr_bmp();
    void _setBackground(int r, int g, int b);                                   // set background color

    void _writePixel(int x, int y, int r, int g, int b);                        // set pixel in bitmap[][]
    void _writeBox(int x1, int y1, int x2, int y2, int r, int g, int b);        // clear Box in bitmap[][]
    void _drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);        // draw line in bitmap[][]
    void _drawCircle(int radius, int r, int g, int b);                          // draw circle in bitmap[][]
    void _drawRadius(int n, int rStart, int rEnd, int r, int g, int b);                  
    int _polarToX(int n, int radius) ;                                          // transform polar coordinates to carthesian coordinate x
    int _polarToY(int n, int radius) ;                                          // transform polar coordinates to carthesian coordinate y
                                                                                // n corresponds to the angle, r to the radius of the polar coordinate system-
                                                                                // range of n = 0,...,239
                                                                                // range of r = 0,...,39

    void _print_16x24(int p_mode, char s[], int xpos, int ypos, int r, int g, int b);
    void _print_12x18(int p_mode, char s[], int xpos, int ypos, int r, int g, int b);
    void _print_10x15(int p_mode, char s[], int xpos, int ypos, int r, int g, int b);
    void _print_icon_30x20(int p_mode, int i_num, int xpos, int ypos, int r, int g, int b);

    int _iconNumber(String iconText);
    String _serverRequest(const char* OpenweatherServer); 

    int _myYear;
    int _myMon;
    int _myMday;
    int _myHour;
    int _myMin;
    int _mySec;
    int _myWday;
    int _myMsec;

    char _ssid[50]; 
    char _ipAddress[50];

    WiFiClient client;                                            // used in connectOpenWeatherMap() and getWeather();

  public:
    unsigned char bitmap[110][165];                                // bitmap with x=110 and y = 110 * 1,5 bytes
    
    int w_icon;                                                  // weather data
    String w_temp;
    String w_humi;
    
    my_BMP();                                                     // Constructor
    void generateBMP(int mode, tm* tm1, const char* ssid, const char* ipAddress);        // generates bitmap according to mode, time, ssid

    void getWeather(String apiKey, String location);
};

#endif
