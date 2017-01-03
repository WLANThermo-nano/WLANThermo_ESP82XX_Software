 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs, Phantomias2006

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    HISTORY:
    0.1.00 - 2016-12-30 initial version
    0.2.00 - 2016-12-30 implement ChannelData
    0.2.01 - 2017-01-02 Change Button Event
    
 ****************************************************/

#include <Wire.h>                 // I2C
#include <SPI.h>                  // SPI
#include <ESP8266WiFi.h>          // WIFI
#include <ESP8266WiFiMulti.h>     // WIFI
#include <WiFiClientSecure.h>     // HTTPS
#include <WiFiUdp.h>              // NTP
#include <TimeLib.h>              // TIME


// ++++++++++++++++++++++++++++++++++++++++++++++++++
// SETTINGS

// HARDEWARE
#ifdef VARIANT_C
  #define CHANNELS 6                  // 4xNTC, 1xKYTPE, 1xSYSTEM
  #define KTYPE 1
  #define MAX1161x_ADDRESS 0x33       // MAX11615
  #define THERMOCOUPLE_CS 16
#elif defined(VARIANT_B)
  #define CHANNELS 7                  // 6xNTC, 1xSYSTEM
  #define MAX1161x_ADDRESS 0x33       // MAX11615
#else 
  #define CHANNELS 3                  // 3xNTC
  #define MAX1161x_ADDRESS 0x34       // MAX11613
#endif

// BATTERY
#define BATTMIN 3600                  // MINIMUM BATTERY VOLTAGE in mV
#define BATTMAX 4185                  // MAXIMUM BATTERY VOLTAGE in mV 
#define ANALOGREADBATTPIN 0
#define BATTDIV 5.9F

// OLED
#define OLED_ADRESS 0x3C              // OLED I2C ADRESS

// TIMER
#define INTERVALBATTERYMODE 1000
#define INTERVALSENSOR 1000
#define INTERVALCOMMUNICATION 30000

// BUS
#define SDA 0
#define SCL 2

// BUTTONS
#define btn_r  4      // Pullup vorhanden
#define btn_l  5      // Pullup vorhanden
#define INPUTMODE INPUT_PULLUP      // INPUT oder INPUT_PULLUP
#define PRELLZEIT 5                 // Prellzeit in Millisekunden   
#define DOUBLECLICKTIME 400         // Längste Zeit für den zweiten Klick beim DOUBLECLICK
#define LONGCLICKTIME 600           // Mindestzeit für einen LONGCLICK
#define MINCOUNTER 0
#define MAXCOUNTER CHANNELS-1

// WIFI
#define APNAME "NEMESIS"
#define APPASSWORD "12345678"


// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// GLOBAL VARIABLES

// CHANNELS
struct ChannelData {
   float temp;            // TEMPERATURE
   int   match;           // Anzeige im Temperatursymbol
   float max;             // MAXIMUM TEMPERATURE
   float min;             // MINIMUM TEMPERATURE
   byte  typ;             // TEMPERATURE SENSOR
   bool  alarm;           // CHANNEL ALARM
   bool  isalarm;         // CURRENT ALARM
};

ChannelData ch[CHANNELS];

String  ttypname[] = {"Maverik",
                      "Fantast-Neu",
                      "100K6A1B",
                      "100K",
                      "SMD NTC",
                      "5K3A1B",
                      "K-Type"};

// SYSTEM
bool  LADEN = false;              // USB POWER SUPPLY?

// OLED
int current_ch = 0;               // CURRENTLY DISPLAYED CHANNEL
int BatteryPercentage = 0;        // BATTERY CHARGE STATE in %
bool LADENSHOW = false;           // LOADING INFORMATION?

// WIFI
byte isAP = 0;                    // WIFI MODE
String wifissid[5];
String wifipass[5];
int lenwifi = 0;
long rssi = 0;                   // Buffer rssi

// BUTTONS
byte buttonPins[]={btn_r,btn_l};          // Pins
#define NUMBUTTONS sizeof(buttonPins)
byte buttonState[NUMBUTTONS];     // Aktueller Status des Buttons HIGH/LOW
enum {NONE, FIRSTDOWN, FIRSTUP, SHORTCLICK, DOUBLECLICK, LONGCLICK};
byte buttonResult[NUMBUTTONS];    // Aktueller Klickstatus der Buttons NONE/SHORTCLICK/LONGCLICK
unsigned long buttonDownTime[NUMBUTTONS]; // Zeitpunkt FIRSTDOWN
int b_counter = 0;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// PRE-DECLARATION

// INIT
void set_serial();                                // Initialize Serial
void set_button();                                // Initialize Buttons
static inline boolean button_input();             // Dectect Button Input
static inline void button_event();                // Response Button Status

// SENSORS
byte set_sensor();                                // Initialize Sensors
int  get_adc_average (byte ch);                   // Reading ADC-Channel Average
double get_thermocouple(void);                    // Reading Temperature KTYPE
void get_Vbat();                                  // Reading Battery Voltage

// TEMPERATURE (TEMP)
float calcT(int r, byte typ);                     // Calculate Temperature from ADC-Bytes
void get_Temperature();                           // Reading Temperature ADC
void set_Channels();                              // Initialize Temperature Channels

// OLED
#include <SSD1306.h>              
#include <OLEDDisplayUi.h>        
SSD1306 display(OLED_ADRESS, SDA, SCL);
OLEDDisplayUi ui     ( &display );

// FRAMES
void drawConnect(int count, int active);          // Frame while system start
void drawLoading();                               // Frame while Loading
void gBattery(OLEDDisplay *display, OLEDDisplayUiState* state);
void drawTemp(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawlimito(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawlimitu(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawtyp(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawalarm(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawwifi(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void set_OLED();                                  // Configuration OLEDDisplay

// FILESYSTEM (FS)
bool loadConfig();                                // Load Config.json at system start
bool setConfig();                                 // Reset config.json to default
bool changeConfig();                              // Set config.json after Change
bool loadWifiSettings();                          // Load wifi.json at system start
bool setWifiSettings();                           // Reset config.json to default
bool addWifiSettings(char* ssid, char* pass);     // Add Wifi Settings to config.json 
void start_fs();                                  // Initialize FileSystem

// MEDIAN
void median_add(int value);                       // add Value to Buffer
void median_sort();                               // sort Buffer
double median_get();                              // get Median from Buffer

// OTA
void set_ota();                                   // Configuration OTA

// WIFI
void set_wifi();                                  // Connect WiFi
void get_rssi();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Serial
void set_serial() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Buttons
void set_button() {
  
  for (int i=0;i<NUMBUTTONS;i++) pinMode(buttonPins[i],INPUTMODE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Dedect Button Input
static inline boolean button_input() {
// Rückgabewert false ==> Prellzeit läuft, Taster wurden nicht abgefragt
// Rückgabewert true ==> Taster wurden abgefragt und Status gesetzt

  static unsigned long lastRunTime;
  //static unsigned long buttonDownTime[NUMBUTTONS];
  unsigned long now=millis();
  
  if (now-lastRunTime<PRELLZEIT) return false; // Prellzeit läuft noch
  
  lastRunTime=now;
  
  for (int i=0;i<NUMBUTTONS;i++)
  {
    byte curState=digitalRead(buttonPins[i]);
    if (INPUTMODE==INPUT_PULLUP) curState=!curState; // Vertauschte Logik bei INPUT_PULLUP
    if (buttonResult[i]>=SHORTCLICK) buttonResult[i]=NONE; // Letztes buttonResult löschen
    if (curState!=buttonState[i]) // Flankenwechsel am Button festgestellt
    {
      if (curState)   // Taster wird gedrückt, Zeit merken
      {
        if (buttonResult[i]==FIRSTUP && now-buttonDownTime[i]<DOUBLECLICKTIME)
          buttonResult[i]=DOUBLECLICK;
        else
        { 
          buttonDownTime[i]=now;
          buttonResult[i]=FIRSTDOWN;
        }
      }
      else  // Taster wird losgelassen
      {
        if (buttonResult[i]==FIRSTDOWN) buttonResult[i]=FIRSTUP;
        if (now-buttonDownTime[i]>=LONGCLICKTIME) buttonResult[i]=LONGCLICK;
      }
    }
    else // kein Flankenwechsel, Up/Down Status ist unverändert
    {
      if (buttonResult[i]==FIRSTUP && now-buttonDownTime[i]>DOUBLECLICKTIME)
        buttonResult[i]=SHORTCLICK;
    }
    buttonState[i]=curState;
  } // for
  return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Response Button Status
static inline void button_event() {

  static unsigned long lastMupiTime;    // Zeitpunkt letztes schnelles Zeppen

  if (buttonResult[0]==LONGCLICK && buttonResult[1]==LONGCLICK) {

      Serial.println("Config Reset");
      setConfig();
      delay(500);
      loadConfig();
    
      return;
  }
  
  // Button 1 gedrückt während man sich im Hauptmenü befindet
  // -> Zum nächsten Kanal switchen
  if (buttonResult[0]==SHORTCLICK && ui.getCurrentFrameCount()==0) {

    b_counter = 0;
    current_ch++;

    if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
    ui.transitionToFrame(0);
    return;
  }

  // Button 1 gedrückt während man im Kontextmenu ist
  // -> Eingabe im Kontextmenu
  if (ui.getCurrentFrameCount()!=0) {

    float tempor;
    int mupi;
    bool event = false;

    // Bei SHORTCLICK kleiner Zahlensprung
    if (buttonResult[0]==SHORTCLICK) {
      mupi = 1;
      event = 1;
    }

    // Bei LONGCLICK großer Zahlensprung jedoch gebremst
    if (buttonResult[0]==FIRSTDOWN && (millis()-buttonDownTime[0]>400)) {
      mupi = 10;
      if (millis()-lastMupiTime > 200) {
        event = 1;
        lastMupiTime = millis();
      }
    }
    
    if (event) {  
      switch (ui.getCurrentFrameCount()) {
        case 1:  // Upper Limit
          tempor = ch[current_ch].max +(0.1*mupi);
          if (tempor > 95.0) tempor = 20.0;
          ch[current_ch].max = tempor;
          break;
        case 2:  // Lower Limit
          tempor = ch[current_ch].min +(0.1*mupi);
          if (tempor > 95.0) tempor = 20.0;
          ch[current_ch].min = tempor;
          break;
        case 3:  // Typ
          tempor = ch[current_ch].typ +1;
          if (tempor > 5) tempor = 0;
          ch[current_ch].typ = tempor;
          break;
        case 4:  // Alarm
          ch[current_ch].alarm = !ch[current_ch].alarm;
          break;
        default:
          break; 
      }
    }
  }
  
  // Button 2 gedrückt: -> Durchs Kontextmenu bewegen
  if (buttonResult[1]==SHORTCLICK) {
    
    b_counter = ui.getCurrentFrameCount();
    
    if (b_counter < 4) { 
      b_counter++; 
    }
    else {
      b_counter = 0;
      changeConfig();
    }
    
    ui.switchToFrame(b_counter);
  }

}

