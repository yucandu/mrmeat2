#include <WiFiManager.h> 
#include <TFT_eSPI.h> 
#include "Fonts/NotoSansBold15.h"
#include <ESP_I2S.h>
// Graphics and font library for ST7735 driver chip
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoOTA.h>
#include "Fonts/SegoeUI_Bold_48.h"
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <SteinhartHart.h>
#include <ESP32I2SAudio.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BackgroundAudio.h>
#include <LittleFS.h>
//#include <JohnnyCash.h>
//#include <eatit.h>
#define STASSID "mikesnet"
#define STAPSK "springchicken"
#include<ADS1115_WE.h> 
#include<Wire.h>
//#include <BackgroundAudioSpeech.h>
//#include <libespeak-ng/voice/en.h>


#define I2C_ADDRESS 0x48
#define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_LARGE SegoeUI_Bold_48

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

bool saved = false;
const char *ssid = STASSID;
const char *pass = STAPSK;

ESP32I2SAudio audio(2, 3, 0); // BCLK, LRCLK, DOUT (,MCLK)

BackgroundAudioWAV BMP(audio);
//BackgroundAudioSpeech BMP(audio);

char auth[] = "DU_j5IxaBQ3Dp-joTLtsB0DM70UZaEDd";

#define rotLpin 21
#define rotRpin 20
#define rotbutton 5
#define onewirepin 7
#define is2connectedthreshold 24000 

OneWire oneWire(onewirepin);
DallasTemperature sensors(&oneWire);

int animStep = 1;
float onewiretempC, tempF, tempA0, tempA1, tempA0f, tempA1f, barx;
long adc0, adc1, adc2, adc3, therm1, therm2, therm3; //had to change to long since uint16 wasn't long enough
float temp1, temp2, temp3;
float volts0, volts1, volts2, volts3;
int channel = 1;
bool onewirefound = false;
bool calibrationMode = false;
bool playSound = false;
bool isPlaying = false;
bool bmp1began = false;
bool bmpdone = true;
int animpos = 80;

SteinhartHart thermistor(15062.08,36874.80,82837.54, 348.15, 323.15, 303.15); //these are the default values for a Weber probe


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite img = TFT_eSprite(&tft);
static  byte abOld;     // Initialize state
volatile int count = 580;     // current rotary count
volatile int downcount = 0;     // current rotary count
volatile int upcount = 0;     // current rotary count
         int old_count;     // old rotary count
volatile int enc_count = 0;

#define TFT_GREY 0x5AEB // New colour

File f;
    const size_t bufsize = 512;
    uint8_t buf[bufsize];

void waitForButtonsReleased() {
  while (!digitalRead(rotbutton)) {
    delay(10); // debounce delay
  }
}

void playWavFromFS(const char* filename) {
  Serial.println("Playing WAV file from LittleFS...");
    f = LittleFS.open(filename, "r");
    if (!f) {
        Serial.println("Failed to open WAV file!");
        return;
    }
    Serial.println("Flushing...");
    BMP.flush();
    Serial.println("Flushed.");

  while (f.available()) {
    // Wait until enough space is available in the buffer
    while (BMP.availableForWrite() < bufsize) {
      delay(1);
    }
    int len = f.read(buf, bufsize);
    if (len > 0) {
      BMP.write(buf, len);
    }
  }
  f.close();
}


float estimateBatteryTime(float voltage) {
  const int numPoints = 22;
  float voltages[numPoints] = {
    3.9735, 3.9558, 3.9230, 3.8958, 3.8647, 3.8223, 3.7902, 3.7655, 3.7163,
    3.6793, 3.6387, 3.5762, 3.5502, 3.5107, 3.4837, 3.4598, 3.4235, 3.3932,
    3.3355, 3.2773, 3.2123, 3.0920
  };
  float times[numPoints] = {
    831, 791, 751, 701, 671, 631, 591, 561, 501,
    471, 431, 391, 351, 311, 261, 241, 191, 151,
    111,  71,  31,   0
  };

  if (voltage >= voltages[0]) return times[0];
  if (voltage <= voltages[numPoints - 1]) return 0;

  for (int i = 0; i < numPoints - 1; i++) {
    if (voltage <= voltages[i] && voltage > voltages[i + 1]) {
      float v1 = voltages[i];
      float v2 = voltages[i + 1];
      float t1 = times[i];
      float t2 = times[i + 1];
      return t1 + (voltage - v1) * (t2 - t1) / (v2 - v1);
    }
  }

  return 0; // fallback
}


  
uint16_t cmap[24];
const char* cmapNames[24];


void initializeCmap() {
    cmap[0] =  TFT_BLACK;
    cmapNames[0] = "BLACK";
    cmap[1] =  TFT_NAVY;
    cmapNames[1] = "NAVY";
    cmap[2] =  TFT_DARKGREEN;
    cmapNames[2] = "DARKGREEN";
    cmap[3] =  TFT_DARKCYAN;
    cmapNames[3] = "DARKCYAN";
    cmap[4] =  TFT_MAROON;
    cmapNames[4] = "MAROON";
    cmap[5] =  TFT_PURPLE;
    cmapNames[5] = "PURPLE";
    cmap[6] =  TFT_OLIVE;
    cmapNames[6] = "OLIVE";
    cmap[7] =  TFT_LIGHTGREY;
    cmapNames[7] = "LIGHTGREY";
    cmap[8] =  TFT_DARKGREY;
    cmapNames[8] = "DARKGREY";
    cmap[9] =  TFT_BLUE;
    cmapNames[9] = "BLUE";
    cmap[10] =  TFT_GREEN;
    cmapNames[10] = "GREEN";
    cmap[11] =  TFT_CYAN;
    cmapNames[11] = "CYAN";
    cmap[12] =  TFT_RED;
    cmapNames[12] = "RED";
    cmap[13] =  TFT_MAGENTA;
    cmapNames[13] = "MAGENTA";
    cmap[14] =  TFT_YELLOW;
    cmapNames[14] = "YELLOW";
    cmap[15] =  TFT_WHITE;
    cmapNames[15] = "WHITE";
    cmap[16] =  TFT_ORANGE;
    cmapNames[16] = "ORANGE";
    cmap[17] =  TFT_GREENYELLOW;
    cmapNames[17] = "GREENYELLOW";
    cmap[18] =  TFT_PINK;
    cmapNames[18] = "PINK";
    cmap[19] =  TFT_BROWN;
    cmapNames[19] = "BROWN";
    cmap[20] =  TFT_GOLD;
    cmapNames[20] = "GOLD";
    cmap[21] =  TFT_SILVER;
    cmapNames[21] = "SILVER";
    cmap[22] =  TFT_SKYBLUE;
    cmapNames[22] = "SKYBLUE";
    cmap[23] =  TFT_VIOLET;
    cmapNames[23] = "VIOLET";
}


double oldtemp, tempdiff, eta, eta2, oldtemp2, tempdiff2;
int etamins, etasecs;

int settemp = 145;
bool is1connected = false;
bool is2connected = false;
int setSelection = 0;
int setAlarm, setUnits;
int setBGC = 15; //background color
int setFGC = 0;
int setVolume = 100;

int rssi;

bool b1pressed = false;
bool b2pressed = false;
bool settingspage = false;
bool kincreased = false;
int setIcons = 1;

Preferences preferences;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160

#define every(interval) \        
  static uint32_t __every__##interval = millis(); \
  if (millis() - __every__##interval >= interval && (__every__##interval = millis()))

  String dallasString,  temp1string,temp2string,temp3string, coeffAstring,  coeffBstring,  coeffCstring;
  
void drawWiFiSignalStrength(int32_t x, int32_t y, int32_t radius) { //chatGPT-generated function to draw a wifi icon with variable rings
    // Get the RSSI value
    
    
    // Define colors
    uint32_t color;
    int numArcs;

    // Determine the color and number of arcs to draw based on RSSI value
    if (rssi > -60) {
        color = cmap[setFGC];
        numArcs = 3;
    } else if (rssi > -75) {
        color = cmap[setFGC];
        numArcs = 2;
    } else if (rssi > -85) {
        color = cmap[setFGC];
        numArcs = 2;
    } else {
        color = cmap[setFGC];
        numArcs = 1;
    }

    // Draw the base circle/dot
    img.fillCircle(x, y+1, 1, color);

    // Draw arcs based on the determined number of arcs and color
    if (numArcs >= 1) {
        img.drawArc(x, y, radius / 3, radius / 3 - 1, 135, 225, color, cmap[setBGC]);  // Arc 1
    }
    if (numArcs >= 2) {
        img.drawArc(x, y, 2 * radius / 3, 2 * radius / 3 - 1, 135, 225, color, cmap[setBGC]);  // Arc 2
    }
    if (numArcs >= 3) {
        img.drawArc(x, y, radius, radius - 1, 135, 225, color, cmap[setBGC]);  // Arc 3
    }
}

double ADSToOhms(float ADSreading) { //convert raw ADS reading to a measured resistance in ohms, knowing R1 is 22000 ohms
      float voltsX = ADSreading;
      return (voltsX * 22000) / (3.3 - voltsX);
}


void drawTemps() { //main screen
 
    if (!digitalRead(rotbutton)) { //if both are pressed at the same time
      settingspage = true;  //go to settings page 
      enc_count = count;
      waitForButtonsReleased();
    }
  

  img.fillSprite(cmap[setBGC]);
  img.setCursor(0,0);
  img.setTextSize(1);
  img.setTextColor(cmap[setFGC], cmap[setBGC]);
  img.setTextDatum(TC_DATUM);

  // Proportional scaling factors
  // 240x240 -> 128x160: x = x * 0.533, y = y * 0.666
  // Example: 240 -> 128, 240 -> 160
    img.loadFont(AA_FONT_LARGE); //smaller font for 128x160
  if (is2connected) {

    float a0ex = 164.4;
    float a1ex = 288.8;
    img.drawFloat(tempA0f, 1, 64, 3);   // 60,5 -> 32,3
    img.drawFloat(tempA1f, 1, 64, 48);   // 180,5 -> 96,3
    //img.drawFastVLine(64, 0, 56, cmap[setFGC]); // 120,0,85 -> 64,0,56
  }
  else if (is1connected) {
    img.drawFloat(tempA0f, 1, 64, 22);   // 115,5 -> 56,3
  }
  else {
    img.drawString("N/C", 64, 22, 1); // 115,5 -> 56,3
  }
  img.drawFastHLine(0, 90, 128, cmap[setFGC]); // 0,85,240 -> 0,56,128
  img.unloadFont();
  img.setTextFont(1);
  img.setTextDatum(TR_DATUM);

  // Degrees/unit indicator
  if (setUnits == 0) {img.drawCircle(120,2,1,cmap[setFGC]); img.drawString("C", 128,1);}
  else if (setUnits == 1) {img.drawCircle(120,2,1,cmap[setFGC]); img.drawString("F", 128,1);}
  else if (setUnits == 2) {img.drawString("K", 127,1);}

  img.setTextDatum(TL_DATUM);
  img.loadFont(AA_FONT_SMALL);
  img.setCursor(3, 110); // 5,100+24 -> 3,70
  String settempstring = ">" + String(count / 4) + "<";
  img.print("Set Temp:");
  img.setTextDatum(TR_DATUM);
  img.drawString(settempstring, 127, 110); // 239,100 -> 127,70
  img.setTextDatum(TL_DATUM);

  img.setCursor(3, 130); // 5,170+24 -> 3,120
  img.print("ETA:");
  String etastring;
  if ((etamins < 1000) && (etamins >= 0)) {
    etastring = String(etamins) + "mins";
  }
  else {
    etastring = "---mins";
  }
  img.setTextDatum(TR_DATUM);
  img.drawString(etastring, 127, 130); // 239,170 -> 127,120
  img.unloadFont();
  img.setTextFont(1);

  img.drawFastHLine(0, 150, 128, cmap[setFGC]); // 0,226,240 -> 0,150,128

  if (setIcons == 0) {
    img.setCursor(1, 153); // 1,231
    img.print(WiFi.localIP());
    String v2String = String(rssi) + "dB/" + String(volts2,2) + "v";
    img.setTextDatum(BR_DATUM);
    img.drawString(v2String, 127,159); // 239,239 -> 127,159
  } else if (setIcons == 1) {
    img.setCursor(1, 152);
    img.print(WiFi.localIP());
    img.setTextDatum(BR_DATUM);
    img.drawRect(110,153,16,6,cmap[setFGC]); // moved right 3 more pixels
    img.fillRect(110,153,barx,6,cmap[setFGC]);
    img.drawFastVLine(126,155,3,cmap[setFGC]);
    if (WiFi.status() == WL_CONNECTED) {drawWiFiSignalStrength(98,157,6);} // 200,237,9 -> 92,157,6
  } else if (setIcons == 2) {
    float minsLeft = estimateBatteryTime(volts2);
    int hours = minsLeft / 60;
    int mins = (int)minsLeft % 60;
    img.setCursor(1, 153);
    char battString[20];
    snprintf(battString, sizeof(battString), "Batt: %dh:%dm", hours, mins);
    img.printf("%s", battString);
    img.setTextDatum(BR_DATUM);
    img.drawRect(110,153,16,6,cmap[setFGC]); // moved right 3 more pixels
    img.fillRect(110,153,barx,6,cmap[setFGC]);
    img.drawFastVLine(126,155,3,cmap[setFGC]);
    if (WiFi.status() == WL_CONNECTED) {drawWiFiSignalStrength(98,157,6);}
  }

  int animX = 80;
  int animY = 156;

  switch (animStep) {
    case 1: // x80, y156
      animX = 80;
      animY = 156;
      break;
    case 2: // x83, y156
      animX = 83;
      animY = 156;
      break;
    case 3: // x83, y159
      animX = 83;
      animY = 159;
      break;
    case 4: // x80, y159
      animX = 80;
      animY = 159;
      break;
    default:
      animX = 80;
      animY = 156;
      break;
  }

  img.fillRect(animX + 3, animY - 3, 3, 3, cmap[setFGC]);

  img.pushSprite(0, 0);
}

volatile bool rotLeft = false; // Flag for left rotation
volatile bool rotRight = false; // Flag for right rotation


void pinChangeISR() {
  enum { upMask = 0x66, downMask = 0x99 };
  byte abNew = (digitalRead(rotRpin) << 1) | digitalRead(rotLpin);
  byte criterion = abNew^abOld;
  if (criterion==1 || criterion==2) {
    if (upMask & (1 << (2*abOld + abNew/2))){
        count++;
    }
      else {count--;       // upMask = ~downMask
      }
    }
  
  abOld = abNew;        // Save new state
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){delay(0);}
  voltage = adc.getRawResult(); // alternative: getResult_mV for Millivolt
  return voltage;
}

float readChannelV(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){delay(0);}
  voltage = adc.getResult_V(); // alternative: getResult_mV for Millivolt
  return voltage;
}

void doADC() {  //take measurements from the ADC without blocking
  if (!adc.isBusy()) {
    if (channel == 0) {
      volts0 = adc.getResult_V();
      adc0 = adc.getRawResult(); // alternative: getResult_mV for Millivolt
      tempA0 = thermistor.resistanceToTemperature(ADSToOhms(volts0));
      if (setUnits == 0) {tempA0f = tempA0 - 273.15;}
      else if (setUnits == 1) {
        tempA0f = ((tempA0 - 273.15) * 1.8) + 32;
      }
      else if (setUnits == 2) {tempA0f = tempA0;}
      adc.setCompareChannels(ADS1115_COMP_1_GND);
      adc.startSingleMeasurement();
      channel = 1;
      return;
    }
    if (channel == 1) {
      volts1 = adc.getResult_V();
      adc1 = adc.getRawResult(); // alternative: getResult_mV for Millivolt
      tempA1 = thermistor.resistanceToTemperature(ADSToOhms(volts1));
      if (setUnits == 0) {tempA1f = tempA1 - 273.15;}
      else if (setUnits == 1) {
        tempA1f = ((tempA1 - 273.15) * 1.8) + 32;
      }
      else if (setUnits == 2) {tempA1f = tempA1;}
      adc.setCompareChannels(ADS1115_COMP_2_GND);
      adc.startSingleMeasurement();
      channel = 2;
      return;
    }
    if (channel == 2) {  //channel for measuring battery voltage
      volts2 = adc.getResult_V() * 2.0;
      adc.setCompareChannels(ADS1115_COMP_0_GND);
      adc.startSingleMeasurement();
      channel = 0;
      return;
    }
  }
}



double mapf(float x, float in_min, float in_max, float out_min, float out_max)  //like default map function but returns float instead of int
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void forceADC() {  //force an ADC measurement and block the CPU to do it
  adc0 = readChannelV(ADS1115_COMP_0_GND);
  adc1 = readChannelV(ADS1115_COMP_1_GND);
  volts2 = readChannelV(ADS1115_COMP_2_GND) * 2.0;  //battery voltage measurement
  tempA0 = thermistor.resistanceToTemperature(ADSToOhms(adc0)) - 273.15;
  tempA0f = (tempA0 * 1.8) + 32;
  tempA1 = thermistor.resistanceToTemperature(ADSToOhms(adc1)) - 273.15;
  tempA1f = (tempA1 * 1.8) + 32;
  barx = mapf (volts2, 3.2, 4.0, 0, 20);
}

uint8_t findDevices(int pin) {
  OneWire ow(pin);

  uint8_t address[8];
  uint8_t count = 0;


  if (ow.search(address))
  {
    Serial.print("\nuint8_t pin");
    Serial.print(pin, DEC);
    Serial.println("[][8] = {");
    do {
      count++;
      Serial.println("  {");
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10) Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7) Serial.print(", ");
      }
      Serial.println("  },");
    } while (ow.search(address));

    Serial.println("};");
    Serial.print("// nr devices found: ");
    Serial.println(count);
  }

  return count;
}

bool editMode = false;
int editSelection = 0; // which menu item is being edited

void drawSettings() {  //if we're in settings mode
  if (old_count > settemp) { //if the encoder was turned left
    rotLeft = true;
  }
  else if (old_count < settemp) { //if the encoder was turned right
    rotRight = true;
  }
  old_count = settemp;
  img.fillSprite(TFT_BLACK);
  img.setCursor(0,0);
  img.setTextSize(1);
  img.setTextColor(TFT_WHITE);
  img.setTextDatum(TL_DATUM);
  img.setTextWrap(true); // Wrap on width
  img.setTextFont(1);  


    if (!digitalRead(rotbutton)) {
      b1pressed = true;
      waitForButtonsReleased();  //wait for button to be released
    }

if (!editMode) {
    if (rotLeft) {
      setSelection--;
      rotLeft = false;  //reset flag
    }
    if (rotRight) {
      setSelection++;
      rotRight = false;  //reset flag
    }
    if (setSelection < 0) setSelection = 7;  //wrap around on the 0th menu item
    if (setSelection > 7) setSelection = 0;  //wrap around on the 8th menu item
    if (b1pressed) {
      editMode = true;
      editSelection = setSelection;
     if (setSelection == 6) { playSound = true; b1pressed = false; editMode = false; }
     if (setSelection == 7) { savePrefs(); settingspage = false; count = enc_count; b1pressed = false; editMode = false; waitForButtonsReleased(); }
   
      b1pressed = false;
      // Optionally, store the base count for this value if you want to "snap" to current value
    }
  } else {
    // In edit mode, use rotLeft/rotRight to change the value
    switch (editSelection) {
      case 0: // Alarm
        if (rotLeft) { setAlarm = (setAlarm + 4) % 5; rotLeft = false; }
        if (rotRight) { setAlarm = (setAlarm + 1) % 5; rotRight = false; }
        break;
      case 1: // Units
        if (rotLeft) { setUnits = (setUnits + 2) % 3; rotLeft = false; }
        if (rotRight) { setUnits = (setUnits + 1) % 3; rotRight = false; }
        break;
      case 2: // BG Colour
        if (rotLeft) { setBGC = (setBGC + 23) % 24; rotLeft = false; }
        if (rotRight) { setBGC = (setBGC + 1) % 24; rotRight = false; }
        break;
      case 3: // FG Colour
        if (rotLeft) { setFGC = (setFGC + 23) % 24; rotLeft = false; }
        if (rotRight) { setFGC = (setFGC + 1) % 24; rotRight = false; }
        break;
      case 4: // Volume
        if (rotLeft) { setVolume = (setVolume + 100) % 101; BMP.setGain(setVolume / 100.0); rotLeft = false; }
        if (rotRight) { setVolume = (setVolume + 1) % 101; BMP.setGain(setVolume / 100.0); rotRight = false; }
        break;
      case 5: // Icons
        if (rotLeft) { setIcons = (setIcons + 2) % 3; rotLeft = false; }
        if (rotRight) { setIcons = (setIcons + 1) % 3; rotRight = false; }
        break;

  // Special actions for Test Spk and Save
    }
    if (b1pressed) {
      editMode = false;
      b1pressed = false;
    }
  }

// Draw menu items
  for (int i = 0; i < 8; i++) {
    if (setSelection == i && !editMode) {
      img.setTextColor(TFT_BLACK, TFT_WHITE, true);
    } else {
      img.setTextColor(TFT_WHITE);
    }
    switch (i) {
      case 0: img.println("Alarm:"); break;
      case 1: img.println("Units:"); break;
      case 2: img.println("BG Colour:"); break;
      case 3: img.println("FG Colour:"); break;
      case 4: img.println("Volume:"); break;
      case 5: img.println("Icons:"); break;
      case 6: img.println(">Test Spk<"); break;
      case 7: img.println(">Save<"); break;
    }
  }

  // Draw values on right, highlight if in edit mode
  img.setTextDatum(TR_DATUM);
  for (int i = 0; i < 8; i++) {
    int y = i * 8;
    bool highlight = (editMode && editSelection == i);
    if (highlight) img.setTextColor(TFT_BLACK, TFT_WHITE, true);
    else img.setTextColor(TFT_WHITE);

    switch (i) {
      case 0: img.drawNumber(setAlarm, 128, y); break;
      case 1: {
        const char* setUnitString = (setUnits == 0) ? "C" : (setUnits == 1) ? "F" : "K";
        img.drawString(setUnitString, 128, y);
        break;
      }
      case 2: img.drawNumber(setBGC, 128, y); break;
      case 3: img.drawNumber(setFGC, 128, y); break;
      case 4: img.drawNumber(setVolume, 128, y); break;
      case 5: {
        const char* setIconString = (setIcons == 0) ? "T" : (setIcons == 1) ? "Y" : "B";
        img.drawString(setIconString, 128, y);
        break;
      }
      case 6: break; // Test Spk, no value
      case 7: break; // Save, no value
    }
  }

  // Special actions for Test Spk and Save
  if (setSelection == 6 && !editMode && b1pressed) { playSound = true; b1pressed = false; editMode = false; }
  if (setSelection == 7 && !editMode && b1pressed) { savePrefs(); b1pressed = false; editMode = false; waitForButtonsReleased(); }

  // Draw sample string
  img.setTextDatum(TC_DATUM);
  img.setTextColor(cmap[setFGC], cmap[setBGC], true);
  char sampleString[32];
  snprintf(sampleString, sizeof(sampleString), "%d%s", setFGC, cmapNames[setFGC]);
  img.drawString(sampleString, 64, 140);

  img.pushSprite(0, 0);
}

// --- drawCalib ---
void drawCalib() {
    img.fillSprite(TFT_MAROON);
    img.setTextSize(1);
    img.setTextColor(TFT_WHITE, TFT_BLACK, true);
    img.setTextWrap(true);
    img.setTextFont(1);
    img.setTextDatum(TL_DATUM);
    img.setCursor(0,0);
    img.println("Calibrating!");
    img.setTextSize(1);
    img.setCursor(0,150);
    img.println("Please wait for all 3 temperature points to be measured...");
    img.setTextSize(1);

    char dallasString[32];
    snprintf(dallasString, sizeof(dallasString), "%.2f C, A0: %.2f", onewiretempC, ADSToOhms(volts0));
    img.drawString(dallasString, 0,20);

    if ((onewiretempC >= 75.0) && (onewiretempC <= 75.2)) {
        temp1 = onewiretempC;
        therm1 = ADSToOhms(volts0);
    }
    if ((onewiretempC >= 50.0) && (onewiretempC <= 50.2)) {
        temp2 = onewiretempC;
        therm2 = ADSToOhms(volts0);
    }
    if ((onewiretempC >= 30.0) && (onewiretempC <= 30.2)) {
        temp3 = onewiretempC;
        therm3 = ADSToOhms(volts0);
    }

    char temp1string[24], temp2string[24], temp3string[24];
    snprintf(temp1string, sizeof(temp1string), "75C = %.2f", therm1);
    snprintf(temp2string, sizeof(temp2string), "50C = %.2f", therm2);
    snprintf(temp3string, sizeof(temp3string), "30C = %.2f", therm3);
    img.drawString(temp1string, 0,40);
    img.drawString(temp2string, 0,60);
    img.drawString(temp3string, 0,80);

    if ((temp3 > 0) && (temp2 > 0) && (temp1 > 0)) {
        if (!saved) {
            thermistor.setTemperature1(temp1 + 273.15);
            thermistor.setTemperature2(temp2 + 273.15);
            thermistor.setTemperature3(temp3 + 273.15);
            thermistor.setResistance1(therm1);
            thermistor.setResistance2(therm2);
            thermistor.setResistance3(therm3);
            thermistor.calcCoefficients();

            char coeffAstring[32], coeffBstring[32], coeffCstring[32];
            snprintf(coeffAstring, sizeof(coeffAstring), "A: %.5f", thermistor.getCoeffA());
            snprintf(coeffBstring, sizeof(coeffBstring), "B: %.5f", thermistor.getCoeffB());
            snprintf(coeffCstring, sizeof(coeffCstring), "C: %.5f", thermistor.getCoeffC());

            preferences.begin("my-app", false);
            preferences.putInt("temp1", temp1);
            preferences.putInt("temp2", temp2);
            preferences.putInt("temp3", temp3);
            preferences.putInt("therm1", therm1);
            preferences.putInt("therm2", therm2);
            preferences.putInt("therm3", therm3);
            preferences.end();
            saved = true;

            img.fillSprite(TFT_GREEN);
            img.setCursor(0,0);
            img.println("Calibration saved, please reboot!");
            img.setTextSize(2);

            img.drawString(temp1string, 0,40);
            img.drawString(temp2string, 0,60);
            img.drawString(temp3string, 0,80);
            img.drawString(coeffAstring, 0,100);
            img.drawString(coeffBstring, 0,120);
            img.drawString(coeffCstring, 0,140);
        }
    }
    img.pushSprite(0, 0);
}

void setup() {
  initializeCmap();
  LittleFS.begin();
  BMP.begin();
  
  
  thermistor.calcCoefficients();
  pinMode(rotLpin, INPUT_PULLUP);
  pinMode(rotRpin, INPUT_PULLUP);
  pinMode(rotbutton, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rotLpin), pinChangeISR, CHANGE);  // Set up pin-change interrupts
  attachInterrupt(digitalPinToInterrupt(rotRpin), pinChangeISR, CHANGE);
    abOld = count = old_count = 0;
  Serial.begin(115200);
  Serial.println("Hello!");
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  Wire.begin();
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_4096); // Set voltage range to 0-4.096V
  adc.setConvRate(ADS1115_8_SPS); 
  Serial.println("Loading prefs");
  preferences.begin("my-app", false); //read preferences from flash, with default values if none are found
  temp1 = preferences.getInt("temp1", 0);
  temp2 = preferences.getInt("temp2", 0);
  temp3 = preferences.getInt("temp3", 0);
  therm1 = preferences.getInt("therm1", 0);
  therm2 = preferences.getInt("therm2", 0);
  therm3 = preferences.getInt("therm3", 0);
  setAlarm = preferences.getInt("setAlarm", 0);
  setUnits = preferences.getInt("setUnits", 1);
  setFGC = preferences.getInt("setFGC", 12);
  setBGC = preferences.getInt("setBGC", 4);
  setVolume = preferences.getInt("setVolume", 100);
  //setLEDmode = preferences.getInt("setLEDmode", 2);
  setIcons = preferences.getInt("setIcons", 1);
  count = preferences.getInt("count", 580);
  preferences.end();
  BMP.setGain(setVolume / 100.0);
  if (setFGC == setBGC) {setFGC = 15; setBGC = 0;}  //do not allow foreground and background colour to be the same
  if ((temp3 > 0) && (temp2 > 0) && (temp1 > 0)) {  //if probe temp calibration has been saved to flash, use it
        thermistor.setTemperature1(temp1 + 273.15);
        thermistor.setTemperature2(temp2 + 273.15);
        thermistor.setTemperature3(temp3 + 273.15);
        thermistor.setResistance1(therm1);
        thermistor.setResistance2(therm2);
        thermistor.setResistance3(therm3);
        thermistor.calcCoefficients();
  }
  img.setColorDepth(8);  //WE DONT HAVE ENOUGH RAM LEFT FOR 16 BIT AND JOHNNY CASH, MUST USE 8 BIT COLOUR!
  img.createSprite(128, 160);
  img.fillSprite(TFT_BLUE);

  oldtemp = tempA0f;  // Initialize oldtemp for future ETA calculations
  oldtemp2 = tempA1f;
  forceADC(); 
  drawTemps();
 Serial.println("Starting wifi");
WiFi.mode(WIFI_STA);  //precharge the wifi
  WiFiManager wm;  //FIRE UP THE WIFI MANAGER SYSTEM FOR WIFI PROVISIONING
  
  if ((!digitalRead(rotbutton)) || !wm.getWiFiIsSaved()){  //If either both buttons are pressed on bootup OR no wifi info is saved
  Serial.println("Resetting stuff");
    //nvs_flash_erase(); // erase the NVS partition to wipe all settings
    //nvs_flash_init(); // initialize the NVS partition.
    preferences.begin("my-app", false); //read preferences from flash, with default values if none are found
     preferences.putInt("temp1", 0);
     preferences.putInt("temp2", 0);
     preferences.putInt("temp3", 0);
     preferences.putInt("therm1", 0);
     preferences.putInt("therm2", 0);
     preferences.putInt("therm3", 0);
     preferences.putInt("setAlarm", 0);
     preferences.putInt("setUnits", 1);
     preferences.putInt("setFGC", 12);
     preferences.putInt("setBGC", 4);
     preferences.putInt("setVolume", 100);
     //preferences.putInt("setLEDmode", 2);
     preferences.putInt("setIcons", 1);
     preferences.putInt("count", 580);
    preferences.end();
    Serial.println("Prefs reset");
    tft.fillScreen(TFT_ORANGE);
    tft.setCursor(0, 0);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.println("SETTINGS RESET.");
    tft.println("");
    tft.println("Please connect to");
    tft.println("'MR MEAT SETUP'");
    tft.println("WiFi, and browse to");
    tft.println("192.168.4.1.");
    wm.resetSettings();  
    

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("MrMeat3 Setup"); 
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    if(!res) {  //if the wifi manager failed to connect to wifi
        tft.fillScreen(TFT_RED);
        tft.setCursor(0, 0);
        tft.println("Failed to connect, restarting");
        delay(3000);
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        tft.fillScreen(TFT_GREEN);
        tft.setCursor(0, 0);
        tft.println("Connected!");
        tft.println(WiFi.localIP());
        delay(3000);
        ESP.restart();  //restart the device now because bugs, user will never notice it's so fuckin fast
    }
  }
  else {  //if wifi information was saved, use it
  Serial.println("Wifi information found, using...");
    WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
  }
  rssi = WiFi.RSSI();
  ArduinoOTA.setHostname("mrmeat3");
  ArduinoOTA.begin();
  
  Serial.println("Connecting blynk...");
  Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
  Blynk.connect();  //Init Blynk
  Serial.println("Blynk connected.");

  sensors.begin();
  sensors.setResolution(11);
  if (findDevices(onewirepin) > 0) //check and see if the probe is connected, if it is,
  {
    onewirefound = true;
    temp1 = 0;  //reset all saved temperatures
    temp2 = 0;
    temp3 = 0;
    calibrationMode = true;  //we're in Calibration town baby
    //temperatureSensors.onTemperatureChange(handleTemperatureChange);
    tft.fillScreen(TFT_PURPLE);
    tft.setCursor(0, 0);
    tft.setTextFont(1);
    tft.println("Calibration Mode!");
    tft.println("To begin, connect 1 meat probe in the left hole, immerse it and the calibration probe in a small cup of hot freshly boiled water (>75C), then press any button.");
    while (digitalRead(rotbutton)) {delay(1);} //wait until a button is pressed
  }
  else {
    Serial.println("No onewire probes found.");
  }



}



void doSound() { //play the sound
  if (playSound) {  //if sound is enabled
  Serial.println("Playing sound");
        playSound = false;
        switch (setAlarm) {
          case 0:
              Serial.println("Playing 0");
              //BMP.end();
              playWavFromFS("/ringoffire.wav");
              break;
          case 1:
              Serial.println("Playing 1");
              //BMP.end();
              playWavFromFS("/weirdal.wav");
              break;
          case 2:
              Serial.println("Playing 2");
              //BMP.end();
              playWavFromFS("/shave.wav");
              break;
          case 3:
              Serial.println("Playing 3");
              //BMP.end();
              playWavFromFS("/suppertime.wav");
              break;
          case 4:
              Serial.println("Playing 4");
              //BMP.end();
              playWavFromFS("/dingfries.wav");
              break;
        }
      }
}

void savePrefs() { //save settings routine
  if (setFGC == setBGC) {setFGC = 15; setBGC = 0;}
  preferences.begin("my-app", false);
  preferences.putInt("setAlarm", setAlarm);
  preferences.putInt("setUnits", setUnits);
  preferences.putInt("setFGC", setFGC);
  preferences.putInt("setBGC", setBGC);
  preferences.putInt("setVolume", setVolume); 
  preferences.putInt("setIcons", setIcons);
 // preferences.putInt("setLEDmode", setLEDmode);
  preferences.putInt("count", count);
  preferences.end();
  
}


void loop() {
  settemp = count / 4; //set the set temperature to the count divided by 4, so we can set it with the rotary encoder
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    Blynk.run();
  }
  doADC();
  doSound(); //play the sound if needed

  every(50){ //every 5 milliseconds, so we don't waste battery power
   if (!calibrationMode) {  //if it's not calibration town
      if (!settingspage) {drawTemps();}  //if we're not in settings mode, draw the main screen
      else {drawSettings();} //else draw the settings screen
      }
    else {
        drawCalib();
      } //else draw the calibration screen
  }

  every(250){
    animStep++;
    if (animStep > 4) {animStep = 1;} //reset the animation step
  }

  if (((tempA0f >= settemp) ||  (tempA1f >= settemp)) && (!calibrationMode) && (is1connected || is2connected) && (setVolume > 0)) {  //If 2nd probe is connected and either temp goes above set temp
    playSound = true;
  }
  
  every(2000) {
     //every 2 seconds update the wifi signal strength variable
    if (calibrationMode) {sensors.requestTemperatures(); onewiretempC = sensors.getTempCByIndex(0);}
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  }



  settemp = count / 4; //set the set temperature to the count divided by 4, so we can set it with the rotary encoder
  if (adc0 < is2connectedthreshold) {is1connected = true;} else {is1connected = false;} //check for probe2 connection
  if (adc1 < is2connectedthreshold) {is2connected = true;} else {is2connected = false;} //check for probe2 connection


  int ETA_INTERVAL = 15;
  every (15000) {  //manually set this to ETA_INTERVAL*1000, can't hardcode due to macro
  if (!calibrationMode) { 
      tempdiff = tempA0f - oldtemp;
      if (is2connected) {  //If 2nd probe is connected, calculate whichever ETA is sooner in seconds
        tempdiff2 = tempA1f - oldtemp2;
        eta = (((settemp - tempA0f)/tempdiff) * ETA_INTERVAL);
        eta2 = (((settemp - tempA1f)/tempdiff2) * ETA_INTERVAL);
        if ((eta2 > 0) && (eta2 < 1000) && (eta2 < eta)) {eta = eta2;}
        oldtemp2 = tempA1f;
      }
      else  //Else if only one probe is connected, calculate the ETA in seconds
      {
        eta = (((settemp - tempA0f)/tempdiff) * ETA_INTERVAL);
      }
      etamins = eta / 60;  //cast it to int and divide it by 60 to get minutes with no remainder, ignore seconds because of inaccuracy
      oldtemp = tempA0f;
    }
  }

  every(30000) {       //every 10 seconds
    
    barx = mapf (volts2, 3.2, 4.0, 0, 20); //update the battery icon length
    if (is2connected) {Blynk.virtualWrite(V4, tempA1f);}
    if (is1connected) {Blynk.virtualWrite(V2, tempA0f);}
    if ((etamins < 1000) && (etamins >= 0)) {
      Blynk.virtualWrite(V6, etamins);
      Blynk.virtualWrite(V7, temperatureRead());
    } 

    Blynk.virtualWrite(V5, volts2);
  }
}
