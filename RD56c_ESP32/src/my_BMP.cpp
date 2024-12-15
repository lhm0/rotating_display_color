// =========================================================================================================================================
//                                                 Rotating Display RD40
//                                                    © Ludwin Monz
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================


#include "my_BMP.h"
#include "FlashFS.h"
#include "my_BMPtemplates.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

my_BMP::my_BMP() {
}


// =========================================================================================================================================
//                                                      generateBMP method
// =========================================================================================================================================

void my_BMP::generateBMP(int mode, tm* tm1, const char* ssid, const char* ipAddress) {      // generates bitmap according to mode
  _myYear = tm1->tm_year;                                       // save tm to private attributes
  _myMon = tm1->tm_mon;
  _myMday = tm1->tm_mday;
  _myHour = tm1->tm_hour;
  _myMin = tm1->tm_min;
  _mySec = tm1->tm_sec;
  _myWday = tm1->tm_wday;

   strcpy(_ssid, ssid);                                        // save ssid to private attribute
   strcpy(_ipAddress, ipAddress);                              // save ipAddress to private attribute

   if (mode==0) _generateIP();                                 // generates bitmap, which shows the IP address
   if (mode==1) _generateAnalog();                             // generates bitmap with analog clock
   if (mode==2) _generateDigital();                            // generates bitmap with digital clock
   if (mode==3) _generateLogoClock();                          // generates bitmap with logo clock
   if (mode==4) _generateWeather();                            // generates bitmap with weather information
   if (mode==5) _generateImage();                            // generates bitmap with image
}

// =========================================================================================================================================
//                                                      _generateIP method
// =========================================================================================================================================

void my_BMP::_generateIP() {
    char text1[100];
    char ip_addr[] = "0.0.0.0";
    char parsed_ip[4][4];
    char ip_part1[20];
    char ip_part2[20];
    char* token;

    _clr_bmp();
    
    _initImageParameters(_bmpIP, "/images/gear.rdc");

    // load rd40 image data
    FlashFS rd40_f("/images/gear.rdc");
    rd40_f.read_f(bitmap, 0, 0, _bmpIP.frameNumber);

    token = strtok(_ipAddress, ".");
    int i = 0;
    while (token != NULL) {
      strcpy(parsed_ip[i], token);
      token = strtok(NULL, ".");
      i++;
    }
    strcpy(ip_part1, parsed_ip[0]);
    strcat(ip_part1, ".");
    strcat(ip_part1, parsed_ip[1]);

    strcpy(ip_part2, parsed_ip[2]);
    strcat(ip_part2, ".");
    strcat(ip_part2, parsed_ip[3]);

    strcpy(text1, ".");
    strcat(ip_part1, text1);    

    _print_10x15(1, ip_part1, 55, 25, 7, 0, 0);
    _print_10x15(1, ip_part2, 55, 9, 7, 0, 0);


    strcpy(text1,_ssid);
    _print_10x15(1, text1, 55, 72, 0, 7, 0);
    
    text1[0]=0x7E;              // Wifi symbol
    text1[1]=0x7F;
    text1[2]=0x00;
    _print_10x15(1, text1, 55, 88, 0, 0, 7);
    
}

// =========================================================================================================================================
//                                                      _generateAnalog() method
// =========================================================================================================================================

void my_BMP::_generateAnalog() {
  
    _loadWatchFace();

    _drawCircle(38, 7, 3, 0);
    _drawCircle(39, 7, 3, 0);
    for (int n=0; n<240; n=n+20) _drawRadius(n, 32, 39, 7, 3, 0);
    for (int n=0; n<240; n=n+4) _drawRadius(n, 35, 39, 7, 3, 0);
    char text[10]="12";
    _print_10x15(1, text, 55, 85, 7, 0, 0);
    strcpy(text,"6");
    _print_10x15(1, text, 55, 10, 7, 0, 0);
    strcpy(text,"3");
    _print_10x15(2, text, 100, 50, 7, 0, 0);
    strcpy(text,"9");
    _print_10x15(1, text, 15, 50, 7, 0, 0); 

  // hour hand
    float hour12;
    if (_myHour>12) hour12=_myHour-12;
    else hour12=_myHour;
    int n = (int)((hour12*60+_myMin)/3);
    _drawRadius(n, 0, 20, 0, 7, 0);

  // minute hand
    n = (int)((_myMin*60+_mySec)/15);
    _drawRadius(n, 0, 35, 0, 7, 0);

  // second hand
    n = _mySec*4;
    _drawRadius(n, 0, 40, 7, 7, 7);
}

void my_BMP::_loadWatchFace() {
//  retrieve logoPath
  _clr_bmp();
  FlashFS wfPath_f{"/variables/clockFacePath.txt"};
  String wfPath = wfPath_f.read_f();
    
  _initImageParameters(_bmpWf, wfPath);

  // load rd40 image data
  FlashFS rd40_f(wfPath);
  rd40_f.read_f(bitmap, 0, 0, _bmpWf.frameNumber);
}


// =========================================================================================================================================
//                                                      _generateDigital() method
// =========================================================================================================================================
void my_BMP::_generateDigital() {
        char text4[64];
        _clr_bmp();                                           // clear bitmap
        sprintf(text4,":");
        _print_12x18(1,text4,40,72, 0, 7, 0);
        _print_12x18(1,text4,70,72, 0, 7, 0);
        sprintf(text4,"%02d",_myHour);
        _print_12x18(1,text4,25,72, 7, 7, 7);
        sprintf(text4,"%02d",_myMin);
        _print_12x18(1,text4,55,72, 7, 7, 7);
        sprintf(text4,"%02d",_mySec);
        _print_12x18(1,text4,85,72, 0, 0, 7);
        sprintf(text4,"%02d.%02d.%04d",_myMday, _myMon+1, _myYear+1900);
        _print_10x15(1,text4,55,24, 0, 7, 0);
        if (_myWday==0) strcpy(text4,"SUN");
        if (_myWday==1) strcpy(text4,"MON");
        if (_myWday==2) strcpy(text4,"TUE");
        if (_myWday==3) strcpy(text4,"WED");
        if (_myWday==4) strcpy(text4,"THU");
        if (_myWday==5) strcpy(text4,"FRI");
        if (_myWday==6) strcpy(text4,"SAT");
        _print_10x15(1,text4,55,7, 0, 0, 7);
}

// =========================================================================================================================================
//                                                      _generateLogoClock() method
// =========================================================================================================================================

void my_BMP::_generateLogoClock() {
    _loadLogo();                                         // copy logo bitmap to "bitmap"                                       
    //_writeBox(12, 72, 98, 102, 4, 4, 4);

    char text4[64];
    sprintf(text4, ":");
    _print_16x24(1,text4,55,70, 0, 0, 7);

    sprintf(text4,"%02d", _myHour);           
    _print_16x24(0,text4,15,70, 0, 0, 7);

    sprintf(text4,"%02d", _myMin);           
    _print_16x24(0,text4,59,70, 0, 0, 7);

    //_writeBox(67, 45, 103, 66, 4, 4, 4);

    sprintf(text4,":%02d", _mySec);          
    _print_10x15(0,text4,70,48, 0, 0, 7);
}

void my_BMP::_loadLogo() {
//  retrieve logoPath
  FlashFS logoPath_f{"/variables/logoPath.txt"};
  String logoPath = logoPath_f.read_f();
  Serial.println(logoPath);
    
  _initImageParameters(_bmpLogo, logoPath);

  // load rd40 image data
  FlashFS rd40_f(logoPath);
  rd40_f.read_f(bitmap, 0, 0, _bmpLogo.frameNumber);
}

// =========================================================================================================================================
//                                                      _generateWeather() method
// =========================================================================================================================================

void my_BMP::_generateWeather() {
 Serial.println("generate Weather...");
String filePath="/html/w_images/";
String filePath2="/html/w_images/";

switch (w_icon) {
  case 1:
    filePath += "02d.rdc";
    filePath2 += "w_img1.rdc";
    break;
  case 2:
    filePath += "03d.rdc";
    filePath2 += "w_img2.rdc";
    break;
  case 3:
    filePath += "04d.rdc";
    filePath2 += "w_img3.rdc";
    break;
  case 4:
    filePath += "09d.rdc";
    filePath2 += "w_img4.rdc";
    break;
  case 5:
    filePath += "10d.rdc";
    filePath2 += "w_img5.rdc";
    break;
  case 6:
    filePath += "11d.rdc";
    filePath2 += "w_img6.rdc";
    break;
  case 7:
    filePath += "13d.rdc";
    filePath2 += "w_img7.rdc";
    break;
  case 8:
    filePath += "50d.rdc";
    break;
  default:
    filePath += "01d.rdc";
    filePath2 += "w_img0.rdc";
    break;
}

Serial.println("filePath: "+filePath);
Serial.println("filePath2: "+filePath2);

_initImageParameters(_bmpWeather, filePath);

FlashFS rd40_f(filePath);
rd40_f.read_f(bitmap, 0, 0, _bmpWeather.frameNumber);

//FlashFS rd40_f2(filePath2);
//rd40_f2.read_f(bitmap, 5, 40, 0);
    
  char text4[64];
  sprintf(text4, ":");
  _print_16x24(1,text4,55,70, 7, 0, 0);
  sprintf(text4,"%02d", _myHour);          
  _print_16x24(0,text4,15,70, 7, 0, 0);
  sprintf(text4,"%02d", _myMin);          
  _print_16x24(0,text4,59,70, 7, 0, 0);
  sprintf(text4,":%02d", _mySec);
  _print_10x15(0,text4,70,48, 7, 0, 0);

  char text5[20];
  w_temp.toCharArray(text5, 20);
  sprintf(text4, "%s'C", text5);        
  _print_10x15(1,text4,55,24, 7, 0, 0);
  w_humi.toCharArray(text5, 20);
  sprintf(text4, "%s%%", text5);        
  _print_10x15(1,text4,55,8, 7, 0, 0);
        
}

// =========================================================================================================================================
//                                                      _generateImage() method
// =========================================================================================================================================

void my_BMP::_generateImage() {
  //  retrieve imagePath
  FlashFS imagePath_f{"/variables/imagePath.txt"};
  String imagePath = imagePath_f.read_f();
  _initImageParameters(_bmpImage, imagePath);

  // load rd40 image data
  FlashFS rd40_f(imagePath);
  rd40_f.read_f(bitmap, 0, 0, _bmpImage.frameNumber);
}

// =========================================================================================================================================
//                                                      clr_bmp method
// =========================================================================================================================================

void my_BMP::_clr_bmp() {
  int xa, ya;
  for (xa=0; xa<110; xa++) {
    for (ya=0; ya<165; ya++) bitmap[xa][ya]=0x00;
  }
}
// =========================================================================================================================================
//                                                      initImageParameters() method
// =========================================================================================================================================


void my_BMP::_initImageParameters(ImageParameters& imageParameters, String imagePath) {
  if (imagePath != imageParameters.fileName) {
    imageParameters.fileName = imagePath;   
    imageParameters.frameNumber = 0;
    File file = SD.open(imagePath.c_str(), "r", false);
    if (!file) {
      Serial.println("Failed to open file for reading");
    } else {
      file.read((uint8_t*)&imageParameters.xdim, 4);         
      file.read((uint8_t*)&imageParameters.ydim, 4);         
      file.read((uint8_t*)&imageParameters.numberOfFrames, 4);  
//      Serial.printf("xdim: %d  ydim: %d  numberOfFrames: %d \n", imageParameters.xdim, imageParameters.ydim, imageParameters.numberOfFrames); 
      file.close();
    }
  } else {
    imageParameters.frameNumber++;
    if (imageParameters.frameNumber >= imageParameters.numberOfFrames) imageParameters.frameNumber = 0;
  }
}


// =========================================================================================================================================
//                                                set background color method
// =========================================================================================================================================

void my_BMP::_setBackground(int r, int g, int b) {
  for (int x=0; x<110; x++) {
    for (int y=0; y<110; y++) _writePixel(x, y, r, g, b);
  }
}

// =========================================================================================================================================
//                        methods for clearing and setting bitmap pixels, clearing bitmap areas and drawing lines
// =========================================================================================================================================

void my_BMP::_writePixel(int x, int y, int r, int g, int b) {
int y_byte, y_start;
  y_byte=3*y/2;
  y_start=3*y%2;                                      // if this value is 0, R is in the lower 4 bits of the byte, otherwise it is in the hogher 4 bits
  if (y_start==0) {
    bitmap[x][y_byte] = (g<<4) | (r & 0x0F);
    bitmap[x][y_byte+1] &= 0xF0;                           // delete lower 4 bits
    bitmap[x][y_byte+1] |= (b & 0X0F);               // replace upper 4 bits with B
  }
  else {
    bitmap[x][y_byte] &= 0x0F;                             // delete the upper 4 bits
    bitmap[x][y_byte] |= (r << 4);                   // replace upper 4 bits with R
    bitmap[x][y_byte+1] = (b << 4) | (g & 0x0F);
  }
}

void my_BMP::_writeBox(int x1, int y1, int x2, int y2, int r, int g, int b) {
  if (x1 > x2) {
    int aux = x1;
    x1 = x2;
    x2 = aux;
  }
  if (y1 > y2) {
    int aux = y2;
    y1 = y2;
    y2 = aux;
  }
  for (int x=x1; x<x2; x++) {
    for (int y=y1; y<y2; y++) _writePixel(x, y, r, g, b);
  }
}

void my_BMP::_drawLine(int x1, int y1, int x2, int y2, int r, int g, int b) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        _writePixel(x1, y1, r, g, b);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void my_BMP::_drawCircle(int radius, int r, int g, int b) {
  int rr = radius;
  for (int n=0; n<240; n++) _writePixel(_polarToX(n, rr), _polarToY(n, rr), r, g, b);
}

void my_BMP::_drawRadius(int n, int rStart, int rEnd, int r, int g, int b) {
  int rr=0;
  for (rr=rStart; rr<rEnd; rr++) {
    int x = _polarToX(n, rr);
    int y = _polarToY(n, rr);
    _writePixel(x, y, r, g, b);
  }
}

int my_BMP::_polarToX(int n, int r) {
  float theta = 0.026179939*(n+0.5);                // 2*pi/240 = 0.026179939
  return (int)roundf(54.5+(r+15.5)*sin(theta));
}

int my_BMP::_polarToY(int n, int r) {
  float theta = 0.026179939*(n+0.5);                // 2*pi/240 = 0.026179939
  return (int)roundf(54.5+(r+15.5)*cos(theta));
}


// =========================================================================================================================================
//                                  methods for printing characters, numbers, and weather icons on bitmap
// =========================================================================================================================================

void my_BMP::_print_16x24(int p_mode, char s[], int xpos, int ypos, int r, int g, int b) {             // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-9*l;
  }
  if (p_mode==2) {
    xpos=xpos-18*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<16; x++) {                                                        // x pixel
        chr=(unsigned long)pgm_read_dword(&chr_16x24[(int)s[j]-0x30][3*x]);       // 24 y bits
        for (int y = 0; y<24; y++) {
          if ((chr & 0x000001) == 1) _writePixel(xpos+j*16+x, ypos+y, r, g, b);
          chr>>=1;
        }
    }
  }
}

void my_BMP::_print_12x18(int p_mode, char s[], int xpos, int ypos, int r, int g, int b) {                      // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-6*l;
  }
  if (p_mode==2) {
    xpos=xpos-12*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<12; x++) {                                                        // x pixel
        chr=(unsigned long)pgm_read_dword(&chr_12x18[(int)s[j]-0x20][3*x]);       // 24 y bits
        for (int y = 0; y<18; y++) {
          if ((chr & 0x000001) == 1) _writePixel(xpos+j*12+x, ypos+y, r, g, b);
          chr>>=1;
        }
    }
  }
}

void my_BMP::_print_10x15(int p_mode, char s[], int xpos, int ypos, int r, int g, int b) {                      // p_mode = print mode
  int l,j,x;                                                                      // p_mode = 0 => right
  unsigned long chr;                                                              // p_mode = 1 => mid
  l= strlen(s);                                                                   // p_mode = 2 => left

  if (p_mode==1) {
    xpos=xpos-5*l;
  }
  if (p_mode==2) {
    xpos=xpos-10*l;
  }

  for (j=0; j<l; j++) {                                                           // char #
    for (x=0; x<10; x++) {                                                        // x pixel
        chr=(unsigned long)pgm_read_dword(&chr_10x15[(int)s[j]-0x20][2*x]);       // 24 y bits
        for (int y = 0; y<15; y++) {
          if ((chr & 1) != 0) _writePixel(xpos+j*10+x, ypos+y, r, g, b);
          chr>>=1;
        }
    }
  }

}

void my_BMP::_print_icon_30x20(int p_mode, int i_num, int xpos, int ypos, int r, int g, int b) {
  int k,x;
  unsigned char chr;
  unsigned long long chr_l=0;

  if (p_mode==1) {
    xpos=xpos-15;
  }
  if (p_mode==2) {
    xpos=xpos-30;
  }

  for (x=0; x<30; x++) {
    chr_l=0;
    for (k=0; k<3; k++) {
      chr_l<<=8;
      chr=pgm_read_byte(&icons_30x20[i_num][x][2-k]);
      chr_l += chr;
    }
    for (int y = 0; y<20; y++) {
      if ((chr_l & 1) != 0) _writePixel(xpos+x, ypos+y, r, g, b);
      chr_l>>=1;
    }
  }


}

// =========================================================================================================================================
//                                  method for dowloading weather data from openweathermap.org
//                      source: https://microcontrollerslab.com/http-get-esp32-arduino-openweathermap-thingspeak-examples/
// =========================================================================================================================================

void my_BMP::getWeather(String apiKey, String location) {

    int status = WiFi.status();
    if (status==WL_CONNECTED) Serial.println("connected to Wifi...");
    else Serial.println("Wifi connection lost...");

    String OpenweatherServer = "http://api.openweathermap.org/data/2.5/weather?q=" + location + ",";
    OpenweatherServer = OpenweatherServer + "&APPID=" + apiKey + "&units=metric";

    String JSONDaten = _serverRequest(OpenweatherServer.c_str());

    DynamicJsonDocument jsonDoc(2000);
    DeserializationError error = deserializeJson(jsonDoc, JSONDaten);    //Deserialize string to AJSON-doc
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    }

    String text1;
    text1 = jsonDoc["weather"][0]["icon"].as<String>();
    w_icon = _iconNumber(text1);        
    w_temp = jsonDoc["main"]["temp"].as<String>();
    w_humi = jsonDoc["main"]["humidity"].as<String>();
  
    float f_temp = round(atof(w_temp.c_str()) * 10) / 10.0;         // round to 1 digit
    char w_temp_c[20];
    sprintf(w_temp_c,"%0.1f",f_temp);
    w_temp = String(w_temp_c);                                      // convert to String

    Serial.print("icon: ");
    Serial.println(w_icon);
    Serial.print("temp: "); 
    Serial.print(w_temp);
    Serial.println(" C");
    Serial.print("humidity: "); 
    Serial.print(w_humi);
    Serial.println(" %"); 

}
String my_BMP::_serverRequest(const char* OpenweatherServer) 
{
  WiFiClient Client;
  HTTPClient httpClient;

  httpClient.begin(Client, OpenweatherServer);

  // send request
  int answerCode = httpClient.GET();

  String serverAnswer = "";

  if (answerCode > 0) 
  {
    // load weather information as string. Will be converted into JSON object later
    serverAnswer = httpClient.getString();
  }

  else 
  {
    Serial.print("ERROR: ");
    Serial.println(serverAnswer);
  }

  httpClient.end();

  return serverAnswer;
}


int my_BMP::_iconNumber(String iconText) {
  int iconNr;
  iconNr=0;
  if (strcmp("01d",iconText.c_str())==0) iconNr=0;
  if (strcmp("01n",iconText.c_str())==0) iconNr=0;
  if (strcmp("02d",iconText.c_str())==0) iconNr=1;
  if (strcmp("02n",iconText.c_str())==0) iconNr=1;
  if (strcmp("03d",iconText.c_str())==0) iconNr=2;
  if (strcmp("03n",iconText.c_str())==0) iconNr=2;
  if (strcmp("04d",iconText.c_str())==0) iconNr=3;
  if (strcmp("04n",iconText.c_str())==0) iconNr=3;
  if (strcmp("09d",iconText.c_str())==0) iconNr=4;
  if (strcmp("09n",iconText.c_str())==0) iconNr=4;
  if (strcmp("10d",iconText.c_str())==0) iconNr=5;
  if (strcmp("10n",iconText.c_str())==0) iconNr=5;
  if (strcmp("11d",iconText.c_str())==0) iconNr=6;
  if (strcmp("11n",iconText.c_str())==0) iconNr=6;
  if (strcmp("13d",iconText.c_str())==0) iconNr=7;
  if (strcmp("13n",iconText.c_str())==0) iconNr=7;
  if (strcmp("50d",iconText.c_str())==0) iconNr=8;
  if (strcmp("50n",iconText.c_str())==0) iconNr=8;
  return iconNr;
}
