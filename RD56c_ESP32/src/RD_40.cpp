// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "RD_40.h"
#include "RD_40trafo.h"

#include <Arduino.h>
#include "FS.h"
#include <SD.h>
#include <SPI.h>

#define FSPI_MOSI 1
#define FSPI_SCLK 3
#define FSPI_MISO 2
#define FSPI_CS 4 



// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

RD_40::RD_40() {
//  brightness = 50;
}

// =========================================================================================================================================
//                                                      begin()
// =========================================================================================================================================
void RD_40::begin() {
  _SPI1.begin(FSPI_SCLK, FSPI_MISO, FSPI_MOSI, FSPI_CS);
  pinMode(FSPI_CS, OUTPUT);
  SPISettings settings1(8000000, MSBFIRST, SPI_MODE0);
  _SPI1.beginTransaction(settings1);
  startPattern(0);
}


// =========================================================================================================================================
//                                                      upload() method
// =========================================================================================================================================

void RD_40::upload(int _brightness) {                                                      // formerly send_lines()    
    brightness=_brightness;
    _line[1920][0][0] = (unsigned char)_brightness;

  for (int k=0; k<3; k++ ) {
    for (int j=0; j<7; j++) {
      for (int i=0; i<1921; i++) {
        digitalWrite(FSPI_CS, LOW);
        _SPI1.transfer(_line[i][j][k]);
        digitalWrite(FSPI_CS, HIGH);
      }
    }
  }
}

// =========================================================================================================================================
//                                                      displayBMP(unsigned char bmp[][14]) method
// =========================================================================================================================================

void RD_40::displayBMP(unsigned char bmp[][165]) {
  int r,a;
  RD_40::pixel P1;
  for (r=0; r<56; r++) {
    for (a=0; a<240; a++) {                   // in the inner of the image, pixel density needs to be reduced. Otherwise, the area is too bright
//      if ((r==0 && a%15!=0) || (r==1 && a%5!=0) || (r==2 && a%3!=0) || (r==3 && a%2!=0) || (r==4 && a%2!=0) || (r==5 && a%2!=0)) {
      if ((r==0 && a%20!=0) || (r==1 && a%10!=0) || (r==2 && a%6!=0) || (r==3 && a%3!=0) || (r==4 && a%3!=0) || (r>=5 && r<7 && a%2!=0)) {
        P1.R=0;
        P1.G=0;
        P1.B=0;
      } else {
        unsigned char x, y;
        x = pgm_read_byte(&trafo_x[r][a]);
        y = pgm_read_byte(&trafo_y[r][a]);
        P1 = _readBmpPix(bmp, x, y);
      }
      _writeLinePix(r, a, P1);
    }
  }
}

// =========================================================================================================================================
//                                                      methods for testing and manipulating bitmap bits
// =========================================================================================================================================

RD_40::pixel RD_40::_readBmpPix(unsigned char bmp[][165], int x, int y) {
  int y_byte, y_start;
  pixel Pixel;
  y_byte=3*y/2;
  y_start=3*y%2;      // if this value is 0, R is in the lower 4 bits of the byte, otherwise it is in the hogher 4 bits
  if (y_start==0) {
    Pixel.R = bmp[x][y_byte] & 0x0F;
    Pixel.G = bmp[x][y_byte]>>4;
    Pixel.B = bmp[x][y_byte+1] & 0x0F;
  }
  else {
    Pixel.R = bmp[x][y_byte]>>4;
    Pixel.G = bmp[x][y_byte+1] & 0x0F;
    Pixel.B = bmp[x][y_byte+1]>>4;
  }
  return Pixel;
}

void RD_40::_writeBmpPix(unsigned char bmp[][165], int x, int y, const pixel& Pixel){
  int y_byte, y_start;
  y_byte=3*y/2;
  y_start=3*y%2;                                      // if this value is 0, R is in the lower 4 bits of the byte, otherwise it is in the hogher 4 bits
  if (y_start==0) {
    bmp[x][y_byte] = (Pixel.G<<4) | (Pixel.R & 0x0F);
    bmp[x][y_byte+1] &= 0xF0;                           // delete lower 4 bits
    bmp[x][y_byte+1] |= (Pixel.B & 0X0F);               // replace upper 4 bits with B
  }
  else {
    bmp[x][y_byte] &= 0x0F;                             // delete the upper 4 bits
    bmp[x][y_byte] |= (Pixel.R << 4);                   // replace upper 4 bits with R
    bmp[x][y_byte+1] = (Pixel.B << 4) | (Pixel.G & 0x0F);
  }
}
   

// =========================================================================================================================================
//                                                      methods for manipulating line bits (=LEDs)
// =========================================================================================================================================

void RD_40::_writeLinePix(int r, int a, const pixel& Pixel) {
// LED#   00 01 02 03 04 05 06 07 | 08 09 10 11 12 13 14 15 | 16 17 18 19 20 21 22 23 | 24 25 26 27 28 29 30 31 | 32 33 34 35 36 37 38 39 | 40 41 42 43 44 45 46 47 | 48 49 50 51 52 53 54 55
// compon 01 29 02 30 03 31 04 32 | 05 33 06 34 07 35 08 36 | 09 37 10 38 11 39 12 40 | 13 41 14 42 15 43 16 44 | 17 45 18 46 19 47 20 48 | 21 49 22 50 23 51 24 52 | 25 53 26 54 27 55 28 56
// byte#   0  3  0  3  0  3  0  3 |  0  4  0  4  0  4  0  4 |  1  4  1  4  1  4  1  4 |  1  5  1  5  1  5  1  5 |  2  5  2  5  2  5  2  5 |  2  6  2  6  2  6  2  6 |  3  6  3  6  3  6  3  6
// bit#    0  4  1  5  2  6  3  7 |  4  0  5  1  6  2  7  3 |  0  4  1  5  2  6  3  7 |  4  0  5  1  6  2  7  3 |  0  4  1  5  2  6  3  7 |  4  0  5  1  6  2  7  3 |  0  4  1  5  2  6  3  7
// bank#   0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1 |  0  1  0  1  0  1  0  1


  unsigned char lineByte[56]={0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x01, 0x04, 0x01, 0x04, 0x01, 0x04, 0x01, 0x04, 0x01, 0x05, 0x01, 0x05, 0x01, 0x05, 0x01, 0x05, 0x02, 0x05, 0x02, 0x05, 0x02, 0x05, 0x02, 0x05, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x03, 0x06, 0x03, 0x06, 0x03, 0x06, 0x03, 0x06};
  unsigned char lineBit[56] ={0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80, 0x10, 0x01, 0x20, 0x02, 0x40, 0x04, 0x80, 0x08, 0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80, 0x10, 0x01, 0x20, 0x02, 0x40, 0x04, 0x80, 0x08, 0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80, 0x10, 0x01, 0x20, 0x02, 0x40, 0x04, 0x80, 0x08, 0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80};
  unsigned char pattern[8][8] = {                                   // parameters are "value" and "bit#"
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0, 1, 0, 0},
    {0, 1, 0, 1, 0, 0, 1, 0},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {1, 0, 1, 0, 1, 1, 0, 1},
    {1, 1, 0, 1, 1, 0, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1}
  };

  a = a*8;
  for (int as=0; as<8; as++) {
    if (pattern[Pixel.R][as]==0) _line[a+as][lineByte[r]][0] &= ~lineBit[r];                              // R
    else _line[a+as][lineByte[r]][0] |= lineBit[r];
    if (pattern[Pixel.G][as]==0) _line[a+as][lineByte[r]][1] &= ~lineBit[r];                              // G
    else _line[a+as][lineByte[r]][1] |= lineBit[r];
    if (pattern[Pixel.B][as]==0) _line[a+as][lineByte[r]][2] &= ~lineBit[r];                              // B
    else _line[a+as][lineByte[r]][2] |= lineBit[r];
  }
} 

void RD_40::startPattern(int angle) {
	int i, j;
	for (i=0; i<(1920); i++) {
		for (j=0; j<7; j++) {
      int test = i + angle;
      unsigned char R, G, B;
      if (test > 1920) test = test-1920;
      if (test<640) {
        R=0xFF;
        G=0;
        B=0;
      }
      if ((test>=640)&(test<1280)) {
        R=0;
        G=0xFF;
        B=0;
      }
      if (test>=1280) {
        R=0;
        G=0;
        B=0xFF;
      }
			_line[i][j][0] = R;							// R
			_line[i][j][1] = G;							// G
			_line[i][j][2] = B;							// B
		}
	}
}