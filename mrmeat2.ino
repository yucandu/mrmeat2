#include <TFT_eSPI.h> 
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <NonBlockingDallas.h>                  //Include the NonBlockingDallas library
#include <Adafruit_ADS1X15.h>
#include <SteinhartHart.h>
#include <Preferences.h>
#include "XT_DAC_Audio.h"
#include "JohnnyCash.h"  
#include <WiFiManager.h> 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <nvs_flash.h>
#include "MusicDefinitions.h"
#include "Fonts/Roboto_Condensed_32.h"
#include "SPIFFS.h"
#include <Arduino_JSON.h>
//#include "Adafruit_MAX1704X.h"
#include <BlynkSimpleEsp32.h>
#include <FastLED.h>
#include <CircularBuffer.h>

#define HISTORY_SIZE 240  // Store 240 readings (1 hour at 15s intervals)

struct Reading {
    float temp1;
    float temp2;
    float setTemp;
    int eta;
    unsigned long timestamp;
};

CircularBuffer<Reading, HISTORY_SIZE> history;

//Adafruit_MAX17048 maxlipo;

#define MYFONT32 &Roboto_Condensed_32

//Settings:
char auth[] = "DU_j5IxaBQ3Dp-joTLtsB0DM70UZaEDd";
#define SPEAKER_PIN 25  //Speaker is using DAC1 pin 25
#define MUTE_PIN 2      //2n3904 transistor to mute speaker is on pin 2
#define ONE_WIRE_BUS 4  //DS18B20 calibration probe is on pin 4   
//#define PWR_LED_PIN 3   //Power LED PWM control is on pin 3
#define button1 17      //Button 1 is on pin 17
#define button2 16      //Button 2 is on pin 16
#define is2connectedthreshold 26300  //raw threshold value measured to decide if probe #2 is connected
#define ETA_INTERVAL 15 //update ETA every 15 seconds
#define LED_PIN 27
#define PWRLED_PIN 19
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
bool speakeron = false;

void waitForButtonsReleased() {
  while (!digitalRead(button1) || !digitalRead(button2)) {
    delay(10); // debounce delay
  }
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



int8_t PROGMEM TwinkleTwinkle[] = {
  NOTE_SILENCE,BEAT_2,NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_G5,NOTE_G5,NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,BEAT_2,
  NOTE_C5,NOTE_C5,NOTE_G5,NOTE_G5,NOTE_A5,NOTE_A5,NOTE_G5,BEAT_2,
  NOTE_F5,NOTE_F5,NOTE_E5,NOTE_E5,NOTE_D5,NOTE_D5,NOTE_C5,BEAT_4,  
  NOTE_SILENCE,BEAT_5,SCORE_END
};

int8_t PROGMEM TwoBits[] = {
  NOTE_SILENCE,BEAT_2,NOTE_C7,BEAT_2,NOTE_G6,NOTE_G6,NOTE_A6,BEAT_2,NOTE_G6,BEAT_2,
  NOTE_SILENCE,BEAT_2,NOTE_B6,BEAT_2,NOTE_C7,BEAT_2,NOTE_SILENCE,SCORE_END
};

int8_t PROGMEM UkranianBellCarol[] = {
  NOTE_SILENCE,BEAT_2,NOTE_DS6,BEAT_2,NOTE_D6,NOTE_DS6,NOTE_C6,BEAT_2,
  NOTE_DS6,BEAT_2,NOTE_D6,NOTE_DS6,NOTE_C6,BEAT_2,
  NOTE_DS6,BEAT_2,NOTE_D6,NOTE_DS6,NOTE_C6,BEAT_2,
  NOTE_DS6,BEAT_2,NOTE_D6,NOTE_DS6,NOTE_C6,BEAT_2,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6,NOTE_DS6, NOTE_F6, NOTE_G6,  NOTE_F6, BEAT_2, NOTE_DS6,BEAT_2,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6,NOTE_DS6, NOTE_F6, NOTE_G6,  NOTE_F6, BEAT_2, NOTE_DS6,BEAT_2,
  NOTE_SILENCE,SCORE_END
};

int8_t PROGMEM AlarmSong[] = {  //simple alarm
  BEAT_1,NOTE_SILENCE,BEAT_1,
  BEAT_4,NOTE_C5,BEAT_4,
  BEAT_4,NOTE_C5,BEAT_4,
  BEAT_4,NOTE_C5,BEAT_4,
  NOTE_SILENCE,BEAT_5,SCORE_END
};

XT_MusicScore_Class Music(TwinkleTwinkle,TEMPO_ALLEGRO,INSTRUMENT_PIANO); 
XT_MusicScore_Class ShaveAndAHaircut(TwoBits,TEMPO_PRESTISSIMO  ,INSTRUMENT_HARPSICHORD ); 
XT_MusicScore_Class DingFriesAreDone(UkranianBellCarol,TEMPO_PRESTISSIMO  , INSTRUMENT_SAXOPHONE  ); 
XT_MusicScore_Class Alarm(AlarmSong,TEMPO_ALLEGRO,  INSTRUMENT_ORGAN );



  
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
bool is2connected = false;
int channel = 0;
int setSelection = 0;
int setAlarm, setUnits, setBGC;
int setFGC = 15;
int setVolume = 100;
int setLEDmode = 0;

bool b1pressed = false;
bool b2pressed = false;
bool settingspage = false;
bool kincreased = false;
int setIcons = 1;


XT_Wav_Class Sound(RingOfFire); 

XT_DAC_Audio_Class DacAudio(SPEAKER_PIN,0);   //Set up the DAC on pin 25

Preferences preferences;


SteinhartHart thermistor(15062.08,36874.80,82837.54, 348.15, 323.15, 303.15); //these are the default values for a Weber probe

Adafruit_ADS1115 ads;  

AsyncWebServer server(80);

AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

float tempC, tempF, tempA0, tempA1, tempA0f, tempA1f;
long adc0, adc1, adc2, adc3, therm1, therm2, therm3; //had to change to long since uint16 wasn't long enough
float temp1, temp2, temp3;
float volts0, volts1, volts2, volts3;

double ADSToOhms(int16_t ADSreading) { //convert raw ADS reading to a measured resistance in ohms, knowing R1 is 22000 ohms
      float voltsX = ads.computeVolts(ADSreading);
      return (voltsX * 22000) / (3.3 - voltsX);
}

String getSensorReadings(){  //JSON constructor

  readings["sensor1"] = String(tempA0f);
  readings["sensor2"] = String(tempA1f);
  readings["sensor3"] = String(settemp);
  readings["sensor4"] = String(etamins);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
    Serial.println("SPIFFS mounted successfully");
  }
}


#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define TIME_INTERVAL 750                      //Time interval among sensor readings [milliseconds]

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


uint8_t findDevices(int pin)
{
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

TFT_eSPI tft = TFT_eSPI();   
TFT_eSprite img = TFT_eSprite(&tft);
int i;
bool dallasConnected = false;
bool calibrationMode = false;
bool saved = false;
String b1String, b2String;
int animpos = 80;
float barx;
int32_t rssi;

//Macro for 'every' 
#define every(interval) \        
  static uint32_t __every__##interval = millis(); \
  if (millis() - __every__##interval >= interval && (__every__##interval = millis()))





double mapf(float x, float in_min, float in_max, float out_min, float out_max)  //like default map function but returns float instead of int
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


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

void drawTemps() { //main screen
  every(100){ //check buttons only every 100ms for debouncing purposes, may introduce lag?
    if (!digitalRead(button1) && !digitalRead(button2)) { //if both are pressed at the same time
      settingspage = true;  //go to settings page 
      waitForButtonsReleased(); // <-- Add this line
    }
    else if (!digitalRead(button1)) {settemp--;}
    else if (!digitalRead(button2)) {settemp++;}
  }
  // ...existing code...
  
  img.fillSprite(cmap[setBGC]); //fill the screen with the background colour
  img.setCursor(0,0);
  img.setTextSize(1);
  img.setTextColor(cmap[setFGC]);
  img.setTextDatum(TC_DATUM);



  if (is2connected) {  //if 2 probes are connected 
    img.setTextFont(6); //use a smaller font
    img.drawFloat(tempA0f, 1, 60,5); //draw both temperatures
    img.drawFloat(tempA1f, 1, 180,5);
    img.drawFastVLine(120,0,85,cmap[setFGC]); //draw a dividing line
  }
  else { //else if only 1 probe is connected
    img.setTextFont(8); //use a larger font
    img.drawFloat(tempA0f, 1, 115,5);  //draw the main temperature
    
  }
  img.drawFastHLine(0,85,240,cmap[setFGC]);  //draw a horizontal line
  img.setTextFont(1);
  img.setTextDatum(TR_DATUM); //draw text from the top right

    if (setUnits == 0) {img.drawCircle(229,2,1,cmap[setFGC]); img.drawString("C", 239,1);} //Celsius
    else if (setUnits == 1) {img.drawCircle(229,2,1,cmap[setFGC]); img.drawString("F", 239,1);}  //Farenheit
    else if (setUnits == 2) {img.drawString("K", 239,1);} //don't draw the degrees symbol if we're in Kelvin

  
  
  img.setTextDatum(TL_DATUM); //draw text from the top left
  img.setFreeFont(MYFONT32);   
  img.setCursor(5,100+24);
  String settempstring = ">" + String(settemp) + "<";
  img.print("Set Temp:"); //draw text where we have setcursor
  img.setTextDatum(TR_DATUM);
  img.drawString(settempstring, 239, 100);  //draw string at explicit location
  img.setTextDatum(TL_DATUM);

  img.setCursor(5,170+24);
  img.print("ETA:"); 
  String etastring;
  if ((etamins < 1000) && (etamins >= 0)) {  //if ETA isn't a wacky number
    etastring = String(etamins) + "mins"; //display it
    }
  else {
    etastring = "---mins"; //otherwise display '---' to indicate it's out of range
  }
  img.setTextDatum(TR_DATUM);
  img.drawString(etastring, 239, 170);
  img.setTextFont(1);


  img.drawFastHLine(0,226,240,cmap[setFGC]); //draw the bottom horizontal line

  if (setIcons == 0) {
    // Text info bar: RSSI and battery voltage
    img.setCursor(1,231);
    img.print(WiFi.localIP());
    String v2String = String(rssi) + "dB/" + String(volts2,2) + "v";
    img.setTextDatum(BR_DATUM);
    img.drawString(v2String, 239,239);
  } else if (setIcons == 1) {
    // Graphical battery and WiFi icons, show IP address
    img.setCursor(1,231);
    img.print(WiFi.localIP());
    img.setTextDatum(BR_DATUM);
    img.drawRect(214,230,20,9,cmap[setFGC]);
    img.fillRect(214,230,barx,9,cmap[setFGC]);
    img.drawFastVLine(234,232,4,cmap[setFGC]);
    img.drawFastVLine(235,232,4,cmap[setFGC]);
    if (WiFi.status() == WL_CONNECTED) {drawWiFiSignalStrength(200,237,9);}
  } else if (setIcons == 2) {
    // Battery time left, graphical battery and WiFi icons
    float minsLeft = estimateBatteryTime(volts2);
    int hours = minsLeft / 60;
    int mins = (int)minsLeft % 60;
    img.setCursor(1,231);
    img.printf("Batt left: %dh:%dmin", hours, mins);
    img.setTextDatum(BR_DATUM);
    img.drawRect(214,230,20,9,cmap[setFGC]);
    img.fillRect(214,230,barx,9,cmap[setFGC]);
    img.drawFastVLine(234,232,4,cmap[setFGC]);
    img.drawFastVLine(235,232,4,cmap[setFGC]);
    if (WiFi.status() == WL_CONNECTED) {drawWiFiSignalStrength(200,237,9);}
  }

  img.fillRect(animpos, 232, 4, 4, cmap[setFGC]); //draw our little status indicator
  animpos += 1; //make it fly
  if (animpos > 160) {animpos = 130;} //make it wrap around

  img.pushSprite(0, 0);  //and DRAW THAT MOTHERFUCKER
}



void drawCalib(){  //if we're in calibration mode
  img.fillSprite(TFT_MAROON); //fill the sprite ya maroon
  img.setTextSize(2);
  img.setTextColor(TFT_WHITE, TFT_BLACK, true);
  img.setTextWrap(true); // Wrap on width
  img.setTextFont(1);
  img.setTextDatum(TL_DATUM);
  img.setCursor(0,0);
  img.println("Calibrating!");
  img.setTextSize(1);
  img.setCursor(0,200);
  img.println("Please wait for all 3 temperature points to be measured...");
  img.setTextSize(2);


   dallasString = String(tempC, 2) + " C, A0: " + String(ADSToOhms(adc0));  //draw the calibration probe temp and meat probe resistance in ohms
  img.drawString(dallasString, 0,20);

  if ((tempC >= 75.0) && (tempC <= 75.2)) { //grab a measurement when we're within 0.2C of 75C, wide range in case we're dropping fast
    temp1 = tempC;  //remember which temperature we used
    therm1 = ADSToOhms(adc0); //and apply this raw meat probe reading
  }
  if ((tempC >= 50.0) && (tempC <= 50.2)) { 
    temp2 = tempC;
    therm2 = ADSToOhms(adc0);
  }
  if ((tempC >= 30.0) && (tempC <= 30.2)) { 
    temp3 = tempC;
    therm3 = ADSToOhms(adc0);
  }

     temp1string = "75C = " +String(therm1);
    img.drawString(temp1string, 0,40);  //draw those readings

     temp2string = "50C = " +String(therm2);
    img.drawString(temp2string, 0,60);

     temp3string = "30C = " +String(therm3);
    img.drawString(temp3string, 0,80);

  if ((temp3 > 0) && (temp2 > 0) && (temp1 > 0)) {  //when we've taken 3 readings

        if (!saved) {  //save it only once
          //digitalWrite(MUTE_PIN, HIGH);  //speaker didn't work in this mode
          thermistor.setTemperature1(temp1 + 273.15);  //save the temperatures and raw readings
          thermistor.setTemperature2(temp2 + 273.15);
          thermistor.setTemperature3(temp3 + 273.15);
          thermistor.setResistance1(therm1);
          thermistor.setResistance2(therm2);
          thermistor.setResistance3(therm3);
          thermistor.calcCoefficients();  //calculate steinhart-hart coefficients for these 3 readings
           coeffAstring = "A: " + String(thermistor.getCoeffA(), 5); //print them
           coeffBstring = "B: " + String(thermistor.getCoeffB(), 5);
           coeffCstring = "C: " + String(thermistor.getCoeffC(), 5);

          preferences.begin("my-app", false); //save them
          preferences.putInt("temp1", temp1);
          preferences.putInt("temp2", temp2);
          preferences.putInt("temp3", temp3);
          preferences.putInt("therm1", therm1);
          preferences.putInt("therm2", therm2);
          preferences.putInt("therm3", therm3);
          preferences.end();
          saved = true;
             // if(Music.Playing==false) {DacAudio.Play(&Music);}
        }
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
  img.pushSprite(0, 0);  //draw it all
}

void drawSettings() {  //if we're in settings mode
     
     
     
  img.fillSprite(TFT_BLACK);
  img.setCursor(0,0);
  img.setTextSize(3);
  img.setTextColor(TFT_WHITE);
  img.setTextDatum(TL_DATUM);
  img.setTextWrap(true); // Wrap on width
  img.setTextFont(1);  
  every(101) {  //button debouncing
    if (!digitalRead(button1)) {b1pressed = true;}  //enter button
    if (!digitalRead(button2)) {setSelection++;} //next button
  }
  if (setSelection > 8) {setSelection = 0;}  //wrap around on the 8th menu item
  if (setSelection == 0) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setAlarm++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("Alarm:");

  if (setSelection == 1) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setUnits++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("Units:");

  if (setSelection == 2) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setBGC++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("BG Colour:");

  if (setSelection == 3) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setFGC++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("FG Colour:");

  if (setSelection == 4) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setVolume++; DacAudio.DacVolume=setVolume; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("Volume:");

  if (setSelection == 5) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setIcons++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("Icons:");

  if (setSelection == 6) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {setLEDmode++; b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println("LED Mode:");

  if (setSelection == 7) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {doSound(); b1pressed = false;}} else {img.setTextColor(TFT_WHITE);}

  img.println(">Test Spk<");

  if (setSelection == 8) {img.setTextColor(TFT_BLACK, TFT_WHITE, true); if (b1pressed) {savePrefs(); b1pressed = false; waitForButtonsReleased();}} else {img.setTextColor(TFT_WHITE);}
  img.println(">Save<");


  img.setTextColor(TFT_WHITE);
  if (setIcons > 2) {setIcons = 0;}  //don't let settings exceed their maximums
  if (setAlarm > 4) {setAlarm = 0;}
  if (setLEDmode > 3) {setLEDmode = 0;}
  if (setUnits > 2) {setUnits = 0; if (kincreased) {settemp-=273; forceADC(); kincreased = false;}}  //change the alarm set temp too when we switch to kelvin so we don't set off the alarm
  if (setBGC > 23) {setBGC = 0;}
  if (setFGC > 23) {setFGC = 0;}
  if (setVolume > 100) {setVolume = 0;}
  img.setCursor(240,0);
  img.setTextDatum(TR_DATUM);
  img.drawNumber(setAlarm, 240, 0);
  String setUnitString;
  String setIconString;
  if (setUnits == 0) {setUnitString = "C";} else if (setUnits == 1) {setUnitString = "F";} else {setUnitString = "K"; if (!kincreased) {settemp+=273; forceADC(); kincreased = true;}}
  if (setIcons == 0) {setIconString = "T";} // T for Text
  else if (setIcons == 1) {setIconString = "Y";} // Y for Yes (icons)
  else if (setIcons == 2) {setIconString = "B";} // B for Battery left
  img.drawString(setUnitString, 240, 24);
  img.drawNumber(setBGC, 240, 24+24);
  img.drawNumber(setFGC, 240, 24+24+24);
  img.drawNumber(setVolume, 240, 24+24+24+24);
  img.drawString(setIconString, 240, 24+24+24+24+24);
  img.drawNumber(setLEDmode, 240, 24+24+24+24+24+24);
  img.setTextDatum(TC_DATUM);
  img.setTextColor(cmap[setFGC], cmap[setBGC], true);
  String sampleString = String(setFGC) +  cmapNames[setFGC];  //draw the text colour sample
  img.drawString(sampleString, 120, 216);
  img.pushSprite(0, 0);  //draw everything
}

void doADC() {  //take measurements from the ADC without blocking
  if (ads.conversionComplete()) {
    if (channel == 0) {
      adc0 = ads.getLastConversionResults();
      if (setUnits == 0) {tempA0f = thermistor.resistanceToTemperature(ADSToOhms(adc0)) - 273.15;}
      else if (setUnits == 1) {
        tempA0 = thermistor.resistanceToTemperature(ADSToOhms(adc0)) - 273.15;
        tempA0f = (tempA0 * 1.8) + 32;
      }
      else if (setUnits == 2) {tempA0f = thermistor.resistanceToTemperature(ADSToOhms(adc0));}

      ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_1, false);
      channel = 1;
      return;
    }
    if (channel == 1) {
      adc1 = ads.getLastConversionResults();
      if (setUnits == 0) {tempA1f = thermistor.resistanceToTemperature(ADSToOhms(adc1)) - 273.15;}
      else if (setUnits == 1) {
        tempA1 = thermistor.resistanceToTemperature(ADSToOhms(adc1)) - 273.15;
        tempA1f = (tempA1 * 1.8) + 32;
      }
      else if (setUnits == 2) {tempA1f = thermistor.resistanceToTemperature(ADSToOhms(adc1));}
      ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_2, false);
      channel = 2;
      return;
    }
    if (channel == 2) {  //channel for measuring battery voltage
      adc2 = ads.getLastConversionResults();
      ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false);
      channel = 0;
      return;
    }
  }
  

  

}

void forceADC() {  //force an ADC measurement and block the CPU to do it
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  volts2 = ads.computeVolts(adc2) * 2.0;  //battery voltage measurement
  //volts2 = maxlipo.cellVoltage();
  //adc0 = ads.getLastConversionResults();
  if (setUnits == 0) {tempA0f = thermistor.resistanceToTemperature(ADSToOhms(adc0)) - 273.15; tempA1f = thermistor.resistanceToTemperature(ADSToOhms(adc1)) - 273.15;}
  else if (setUnits == 1) {
    tempA0 = thermistor.resistanceToTemperature(ADSToOhms(adc0)) - 273.15;
    tempA0f = (tempA0 * 1.8) + 32;
    tempA1 = thermistor.resistanceToTemperature(ADSToOhms(adc1)) - 273.15;
    tempA1f = (tempA1 * 1.8) + 32;
  }
  else if (setUnits == 2) {tempA0f = thermistor.resistanceToTemperature(ADSToOhms(adc0)); tempA1f = thermistor.resistanceToTemperature(ADSToOhms(adc1));}
  barx = mapf (volts2, 3.2, 4.0, 0, 20);
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
  preferences.putInt("setLEDmode", setLEDmode);
  preferences.putInt("settemp", settemp);
  preferences.end();
  settingspage = false;
}

void doSound() { //play alarm 
if (setVolume > 0) {digitalWrite(MUTE_PIN, HIGH);}
  if (setAlarm == 0) {
    if(Sound.Playing==false)       
    DacAudio.Play(&Sound);
  }
  else if (setAlarm == 1) {
    if(Music.Playing==false)       
    DacAudio.Play(&Music);
  }
  else if (setAlarm == 2) {
    if(Alarm.Playing==false)       
    DacAudio.Play(&Alarm);
  }
  else if (setAlarm == 3) {
    if(ShaveAndAHaircut.Playing==false)       
    DacAudio.Play(&ShaveAndAHaircut);
  }
  else if (setAlarm == 4) {
    if(DingFriesAreDone.Playing==false)       
    DacAudio.Play(&DingFriesAreDone);
  }
}

void setup() {  //do this on bootup
//setCpuFrequencyMhz(160);
  Serial.begin(115200);
  Serial.println("Hello!");
  initializeCmap();  //do the color map thing
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
  tft.setTextWrap(true); // Wrap on width
  tft.setTextFont(2);
  tft.setTextSize(1);
    tft.fillScreen(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.println("Hello.");
    tft.println("");
  img.setColorDepth(8);  //WE DONT HAVE ENOUGH RAM LEFT FOR 16 BIT AND JOHNNY CASH, MUST USE 8 BIT COLOUR!
  img.createSprite(240, 240);
  img.fillSprite(TFT_BLUE);
  
  DacAudio.StopAllSounds();
  
  /*if (!maxlipo.begin()) {
    Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  
  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x")); 
  Serial.println(maxlipo.getChipID(), HEX);
  maxlipo.setResetVoltage(2.5);*/

  ads.begin();  //fire up the ADC
  ads.setGain(GAIN_ONE);  //1x gain setting is perfect
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false);
  pinMode(MUTE_PIN, OUTPUT);
  pinMode(PWRLED_PIN, OUTPUT);
  digitalWrite(PWRLED_PIN, HIGH);  //turn on the power LED

  
  digitalWrite(MUTE_PIN, LOW);  //mute the speaker when we're not playing anything so we don't hear the wifi hiss
  leds[0] = CRGB(0, 0, 255);
FastLED.show(); //turn on the red LED
  pinMode(button1, INPUT_PULLUP); //use internal pullup resistors for the switches so we don't have to solder any
  pinMode(button2, INPUT_PULLUP);
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
  setLEDmode = preferences.getInt("setLEDmode", 2);
  setIcons = preferences.getInt("setIcons", 1);
  settemp = preferences.getInt("settemp", 145);
  preferences.end();
  DacAudio.DacVolume=setVolume;
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
  rssi = WiFi.RSSI();
  forceADC();  // Make sure we have valid temperature readings
  Reading firstReading = {
      tempA0f,
      tempA1f,
      (float)settemp,
      0,  // Initial ETA of 0 since we don't have oldtemp yet
      millis()
  };
  history.push(firstReading);
  oldtemp = tempA0f;  // Initialize oldtemp for future ETA calculations
  oldtemp2 = tempA1f;
  drawTemps();  //draw one bogus temperature screen while we're booting up

  WiFi.mode(WIFI_STA);  //precharge the wifi
  WiFiManager wm;  //FIRE UP THE WIFI MANAGER SYSTEM FOR WIFI PROVISIONING
  if ((!digitalRead(button1) && !digitalRead(button2)) || !wm.getWiFiIsSaved()){  //If either both buttons are pressed on bootup OR no wifi info is saved
    nvs_flash_erase(); // erase the NVS partition to wipe all settings
    nvs_flash_init(); // initialize the NVS partition.
    tft.fillScreen(TFT_ORANGE);
    tft.setCursor(0, 0);
    tft.setTextFont(4);
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
    res = wm.autoConnect("Mr Meat Setup"); 

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
        leds[0] = CRGB(0, 0, 255);
FastLED.show(); 
        delay(250);
        leds[0] = CRGB(0, 0, 0);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 255);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 0);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 255);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 0);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 255);
FastLED.show();
        delay(250);
        leds[0] = CRGB(0, 0, 0);
FastLED.show();
        delay(250);
        tft.fillScreen(TFT_BLACK);
        ESP.restart();  //restart the device now because bugs, user will never notice it's so fuckin fast
    }
  }
  else {  //if wifi information was saved, use it
    WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());

  }
  leds[0] = CRGB(0, 0, 255);
FastLED.show(); //make sure red power LED is still on
  initSPIFFS();
  sensors.begin();
  if (findDevices(ONE_WIRE_BUS) > 0) //check and see if the probe is connected, if it is,
  {
    temp1 = 0;  //reset all saved temperatures
    temp2 = 0;
    temp3 = 0;
    calibrationMode = true;  //we're in Calibration town baby
    //temperatureSensors.onTemperatureChange(handleTemperatureChange);
    tft.fillScreen(TFT_PURPLE);
    tft.setCursor(0, 0);
    tft.setTextFont(4);
    tft.println("Calibration Mode!");
    tft.println("To begin, connect 1 meat probe in the left hole, immerse it and the calibration probe in a small cup of hot freshly boiled water (>75C), then press any button.");
    while (digitalRead(button1) && digitalRead(button2)) {} //wait until a button is pressed
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){  //If someone connects to the root of our HTTP site, serve index.html
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){  //Serve the raw JSON data on /readings
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
    JSONVar historyArray;
    const int maxPoints = 120; // Limit to 30 minutes of data (120 points at 15s intervals)
    
    int startIdx = max(0, (int)history.size() - maxPoints);
    int count = 0;
    
    for(unsigned int i = startIdx; i < history.size() && count < maxPoints; i++) {
        JSONVar reading;
        reading["sensor1"] = String(history[i].temp1);
        reading["sensor2"] = String(history[i].temp2);
        reading["sensor3"] = String(history[i].setTemp);
        reading["sensor4"] = String(history[i].eta);
        reading["timestamp"] = history[i].timestamp;
        historyArray[count++] = reading;
    }
    
    String jsonString = JSON.stringify(historyArray);
    request->send(200, "application/json", jsonString);
    jsonString = String(); // Free the string
    historyArray = JSONVar(); // Clear the JSONVar
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  AsyncElegantOTA.begin(&server);  //Start the OTA firmware updater on /update
  server.begin(); //begin all webservers
  Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
  Blynk.connect();  //Init Blynk
  //maxlipo.quickStart();
} //end of setup routine

void loop() {  //do this all the time, main CPU loop
  Blynk.run();
  //temperatureSensors.update(); //check for new DS18b20 calibration probe temperature update
  doADC(); //update meat probe temperatures
  every(2000) {
    rssi = WiFi.RSSI(); //every 2 seconds update the wifi signal strength variable
    if (calibrationMode) {sensors.requestTemperatures(); tempC = sensors.getTempCByIndex(0);}
  }
  every(10000) {       //every 10 seconds
    volts2 = ads.computeVolts(adc2) * 2.0; //update the battery voltage
    //volts2 = maxlipo.cellVoltage();
    barx = mapf (volts2, 3.2, 4.0, 0, 20); //update the battery icon length
    events.send("ping",NULL,millis());  //update the web interface
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());

    if (is2connected) {Blynk.virtualWrite(V4, tempA1f);}
    Blynk.virtualWrite(V2, tempA0f);
    Blynk.virtualWrite(V5, volts2);
    //Blynk.virtualWrite(V11, maxlipo.chargeRate());
    //Blynk.virtualWrite(V12, maxlipo.cellPercent());
  }

  every(5){ //every 5 milliseconds, so we don't waste battery power
    
    if (adc1 < is2connectedthreshold) {is2connected = true;} else {is2connected = false;} //check for probe2 connection
    if (!calibrationMode) {  //if it's not calibration town
      if (setLEDmode == 0) {leds[0] = CRGB(0, 0, 0);
        FastLED.show();}  //configure the red power LED based on settings
      else if (setLEDmode == 1) {leds[0] = CRGB(0, 0, 40);
        FastLED.show();}
      else if (setLEDmode == 2) {leds[0] = CRGB(0, 0, 255);
        FastLED.show();}
      else if (setLEDmode == 3) {  //the fancy 4th setting
          float led_brightness = mapf(tempA0f, 0, settemp, 0, 255); 
          leds[0] = CRGB(led_brightness, 0, 0);
          FastLED.show();
        }
      

      if (!settingspage) {drawTemps();}  //if we're not in settings mode, draw the main screen
      else {drawSettings();} //else draw the settings screen
      }
    else {
      drawCalib();
      } //else draw the calibration screen
  }
  DacAudio.FillBuffer(); //be constantly filling the memory with audio so it's ready to play without stutters on demand

  if (((Sound.Playing) || (Music.Playing) || (Alarm.Playing) || (DingFriesAreDone.Playing) || (ShaveAndAHaircut.Playing)) && (setVolume > 0)) {  //if we're playing anything
    if (!speakeron) {
      digitalWrite(MUTE_PIN, HIGH); 
      speakeron = true;
    }  //turn on the 2n3904 transistor to unmute the speaker
  }
  else  {
    if (speakeron) {
      digitalWrite(MUTE_PIN, LOW);
      speakeron = false;
    } //otherwise mute it
  }

  if (((tempA0f >= settemp) ||  (tempA1f >= settemp)) && (!calibrationMode)) {  //If 2nd probe is connected and either temp goes above set temp
    doSound(); //sound the alarm
  }

  

  every (15000) {  //manually set this to ETA_INTERVAL*1000, can't hardcode due to macro
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
    Reading newReading = {
      tempA0f,
      tempA1f,
      (float)settemp,
      etamins,
      millis()
    };
    history.push(newReading);

  }

} //end of main CPU loop
