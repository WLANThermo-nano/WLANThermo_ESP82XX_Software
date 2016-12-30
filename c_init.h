 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

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
    
 ****************************************************/

#include <Wire.h>                 // I2C
#include <SPI.h>                  // SPI
#include <ESP8266WiFi.h>          // WIFI
#include <ESP8266WiFiMulti.h>     // WIFI
#include <WiFiClientSecure.h>     // HTTPS
#include <WiFiUdp.h>              // NTP


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
#define BATTMIN 3400                  // MINIMUM BATTERY VOLTAGE in mV
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
#define BUT1  4      // Pullup vorhanden
#define BUT2  5      // Pullup vorhanden
#define MINCOUNTER 0
#define MAXCOUNTER CHANNELS-1

// WIFI
#define APNAME "NEMESIS"
#define APPASSWORD "12345678"


// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// GLOBAL VARIABLES

// CHANNELS
float   temp[CHANNELS];           // TEMPERATURE ARRAY
int     match[CHANNELS];          // Anzeige im Temperatursymbol

float   tmax[CHANNELS];           // MAXIMUM TEMPERATURE ARRAY
float   tmin[CHANNELS];           // MINIMUM TEMPERATURE ARRAY
byte    ttyp[CHANNELS];           // TEMPERATURE SENSOR TYPE ARRAY
boolean talarm[CHANNELS];         // CHANNEL ALARM ARRAY

boolean alarm[CHANNELS];
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

// BUTTONS
volatile int but1_flag;     // Variable liegt im RAM nicht im REGISTER
volatile int but2_flag;     // Variable liegt im RAM nicht im REGISTER
volatile int but1_rst; 
volatile int but2_rst; 
int b_counter = 0;
int mupi = 1;               // Multiplier
long rssi = 0;              // Buffer rssi

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// PRE-DECLARATION

// INIT
void set_serial();                                // Initialize Serial
void set_button();                                // Initialize Button Event
void button_get();                                // Button Event

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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Serial
void set_serial() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Button Flags
void but1_event() {
  but1_flag = true;
}

void but1_long() {
  but1_rst = true;
}

void but2_event() {
  but2_flag = true;
}

void but2_long() {
  but2_rst = true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Button_Event
void set_button() {

  but1_flag = false;
  but2_flag = false;
  but1_rst = false;
  but2_rst = false;
  
  pinMode(BUT1, INPUT);
  pinMode(BUT2, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUT1),but1_event,FALLING);
  attachInterrupt(digitalPinToInterrupt(BUT2),but2_event,FALLING);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Button_Event
void button_get() {

  bool event1 = false;
  bool event2 = false;
    
  if (but1_flag) {
    delay(400); // Doppelschlag vermeiden
    but1_flag = false;
    event1 = true;
  }
  if (but2_flag) {
    delay(400); // Doppelschlag vermeiden
    but2_flag = false;
    event2 = true;
  }

  if (!digitalRead(BUT1) && !digitalRead(BUT2)) {
    
    setConfig();
    delay(500);
    loadConfig();
    Serial.println("Config Reset");
    delay(2000);  // Falls länger gedrückt wird
    
    return;
  }
  
  if (event1) {

    // Zum nächsten Kanal switchen
    if (ui.getCurrentFrameCount()==0) {
    
      b_counter = 0;
      current_ch++;

      if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
      ui.transitionToFrame(0);
    }

    // Eingabe im Kontextmenu
    else {
      
      float tempor;
          
      if (!digitalRead(BUT1)) {  // Button wird gedrückt gehalten
        mupi = 10;
        but1_flag = true;
      }
      else if (mupi == 10) {  // falls letzter Aufruf während Button gehalten
        mupi = 1;
        return;
      }
      
      switch (ui.getCurrentFrameCount()) {
        case 1:  // Upper Limit
          tempor = tmax[current_ch] +(0.1*mupi);
          if (tempor > 95.0) tempor = 20.0;
          tmax[current_ch] = tempor;
          break;
        case 2:  // Lower Limit
          tempor = tmin[current_ch] +(0.1*mupi);
          if (tempor > 95.0) tempor = 20.0;
          tmin[current_ch] = tempor;
          break;
        case 3:  // Typ
          tempor = ttyp[current_ch] +1;
          if (tempor > 5) tempor = 0;
          ttyp[current_ch] = tempor;
          break;
        case 4:  // Alarm
          talarm[current_ch] = !talarm[current_ch];
          break;
        default:
          break; 
      }
    }
  }

  // Durchs Kontextmenu bewegen
  if (event2) {
    
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








