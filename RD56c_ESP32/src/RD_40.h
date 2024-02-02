// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


// ******************************************************************************************************************************************
//
//   This class provides the rotating display object. It consists of the display data (one bit per LED) and of methods for manipulating
//   the data and for transferring it to the display controller.
//
// ******************************************************************************************************************************************


#ifndef RD_40_H
#define RD_40_H

#include <Arduino.h>
#include "FS.h"
#include <SD.h>
#include <SPI.h>

class RD_40 {
  private:
    struct pixel {
        uint8_t R;
        uint8_t G;
        uint8_t B;
    };
  
    SPIClass _SPI1 = SPIClass(FSPI);

    unsigned char _line[1921][7][3];                              // 1920 lines, 7 bytes x 8 bits = 56 LEDs, 3 colors
                                                                  // 1921. provides space for transfer of brightness to rp

    pixel _readBmpPix(unsigned char bmp[][165], int x, int y);                          // read pixel values of bitmap
    void _writeBmpPix(unsigned char bmp[][165], int x, int y, const pixel& Pixel);      // write pixel values of bitmap

    void _writeLinePix(int r, int a, const pixel& Pixel);                               // write line values
    bool help=true;
  
  public:

    int brightness;                                               // brightness level in %
  
    RD_40();                                                      // constructor
    void begin();                                                 // begin function initiates VSPI

    void upload(int _brightness);                                 // uploads display image data and brightness setting to display controller
    void displayBMP(unsigned char bmp[][165]);                     // displays a bitmap
    void startPattern(int angle);

};

#endif