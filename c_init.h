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
    0.2.01 - 2017-01-02 change button events
    0.2.02 - 2017-01-04 add inactive and temperatur unit
    0.2.03 - 2017-01-04 add button events
    
 ****************************************************/

#include <Wire.h>                 // I2C
#include <SPI.h>                  // SPI
#include <ESP8266WiFi.h>          // WIFI
#include <ESP8266WiFiMulti.h>     // WIFI
#include <WiFiClientSecure.h>     // HTTPS
#include <WiFiUdp.h>              // NTP
#include <TimeLib.h>              // TIME
#include <EEPROM.h>               // EEPROM
#include <FS.h>                   // FILESYSTEM
#include <ArduinoJson.h>          // JSON

extern "C" {
#include "user_interface.h"
#include "spi_flash.h"
}

extern "C" uint32_t _SPIFFS_start;      // START ADRESS FS
extern "C" uint32_t _SPIFFS_end;        // FIRST ADRESS AFTER FS

// ++++++++++++++++++++++++++++++++++++++++++++++++++
// SETTINGS

// HARDWARE
#define FIRMWAREVERSION "V0.3"

// CHANNELS
#define CHANNELS 8                     // UPDATE AUF HARDWARE 4.05
#define INACTIVEVALUE  999             // NO NTC CONNECTED
#define SENSORTYPEN    7               // NUMBER OF SENSORS
#define LIMITUNTERGRENZE -20           // MINIMUM LIMIT
#define LIMITOBERGRENZE 999            // MAXIMUM LIMIT
#define MAX1161x_ADDRESS 0x33          // MAX11615

// BATTERY
#define BATTMIN 3600                  // MINIMUM BATTERY VOLTAGE in mV
#define BATTMAX 4185                  // MAXIMUM BATTERY VOLTAGE in mV 
#define ANALOGREADBATTPIN 0           // INTERNAL ADC PIN
#define BATTDIV 5.9F
#define CHARGEDETECTION 16              // LOAD DETECTION PIN

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
#define btn_r  4                    // Pullup vorhanden
#define btn_l  5                    // Pullup vorhanden
#define INPUTMODE INPUT_PULLUP      // INPUT oder INPUT_PULLUP
#define PRELLZEIT 5                 // Prellzeit in Millisekunden   
#define DOUBLECLICKTIME 400         // Längste Zeit für den zweiten Klick beim DOUBLECLICK
#define LONGCLICKTIME 600           // Mindestzeit für einen LONGCLICK
#define MINCOUNTER 0
#define MAXCOUNTER CHANNELS-1

// WIFI
#define APNAME "NANO-AP"
#define APPASSWORD "12345678"

// FILESYSTEM
#define CHANNELJSONVERSION 4        // FS VERSION
#define EEPROM_SIZE 1536            // EEPROM SIZE

// PITMASTER
#define PITMASTER1 15               // PITMASTER PIN
#define PITMIN 0                    // LOWER LIMIT SET
#define PITMAX 100                  // UPPER LIMIT SET
#define PITMASTERSIZE 5             // PITMASTER SETTINGS LIMIT

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// GLOBAL VARIABLES

// CHANNELS
struct ChannelData {
   String name;           // CHANNEL NAME
   float temp;            // TEMPERATURE
   int   match;           // Anzeige im Temperatursymbol
   float max;             // MAXIMUM TEMPERATURE
   float min;             // MINIMUM TEMPERATURE
   byte  typ;             // TEMPERATURE SENSOR
   bool  alarm;           // CHANNEL ALARM
   bool  isalarm;         // CURRENT ALARM
   String color;          // COLOR
};

ChannelData ch[CHANNELS];

String  ttypname[SENSORTYPEN] = {"Maverick",
                      "Fantast-Neu",
                      "100K6A1B",
                      "100K",
                      "SMD NTC",
                      "5K3A1B",
                      "Typ K"};

String  temp_unit = "C";
String colors[8] = {"#6495ED", "#CD2626", "#66CDAA", "#F4A460", "#D02090", "#FFEC8B", "#BA55D3", "#008B8B"};

// PITMASTER
struct Pitmaster {
   byte typ;           // PITMASTER NAME/TYP
   float set;            // SET-TEMPERATUR
   bool  active;           // IS PITMASTER ACTIVE
   byte  channel;         // PITMASTER CHANNEL
   float value;           // PITMASTER VALUE IN %
   int manuel;            // MANUEL PITMASTER VALUE IN %
   bool event;
   int16_t msec;          // PITMASTER VALUE IN MILLISEC
   unsigned long last;
};

Pitmaster pitmaster;
int pidsize;

// DATALOGGER
struct datalogger {
 uint16_t tem[8];
 long timestamp;
};

#define MAXLOGCOUNT 204             // SPI_FLASH_SEC_SIZE/ sizeof(datalogger)
datalogger mylog[MAXLOGCOUNT];
datalogger archivlog[MAXLOGCOUNT];
int log_count = 0;
uint32_t log_sector;                // erster Sector von APP2
uint32_t freeSpaceStart;            // First Sector of OTA
uint32_t freeSpaceEnd;              // Last Sector+1 of OTA

// SYSTEM
bool stby = false;                // USB POWER SUPPLY?
bool doAlarm = false;             // HARDWARE ALARM
bool charge = false;              // CHARGE DETECTION

// OLED
int current_ch = 0;               // CURRENTLY DISPLAYED CHANNEL
int BatteryPercentage = 0;        // BATTERY CHARGE STATE in %
bool LADENSHOW = false;           // LOADING INFORMATION?
bool INACTIVESHOW = true;         // SHOW INACTIVE CHANNELS
bool displayblocked = false;                     // No OLED Update
enum {NO, CONFIGRESET, CHANGEUNIT, OTAUPDATE, HARDWAREALARM};
int question = NO;                               // Which Question;

// FILESYSTEM
enum {eCHANNEL, eWIFI, eTHING, ePIT, ePRESET};

// WIFI
byte isAP = 0;                    // WIFI MODE
String wifissid[5];
String wifipass[5];
int lenwifi = 0;
long rssi = 0;                   // Buffer rssi
String THINGSPEAK_KEY;

// BUTTONS
byte buttonPins[]={btn_r,btn_l};          // Pins
#define NUMBUTTONS sizeof(buttonPins)
byte buttonState[NUMBUTTONS];     // Aktueller Status des Buttons HIGH/LOW
enum {NONE, FIRSTDOWN, FIRSTUP, SHORTCLICK, DOUBLECLICK, LONGCLICK};
byte buttonResult[NUMBUTTONS];    // Aktueller Klickstatus der Buttons NONE/SHORTCLICK/LONGCLICK
unsigned long buttonDownTime[NUMBUTTONS]; // Zeitpunkt FIRSTDOWN
int b_counter = 0;
byte menu_count = 0;                      // Counter for Menu
byte inMenu = 1;
enum {MAINMENU, TEMPSUB, TEMPKONTEXT, PITSUB, SYSTEMSUB};


// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// PRE-DECLARATION

// INIT
void set_serial();                                // Initialize Serial
void set_button();                                // Initialize Buttons
static inline boolean button_input();             // Dectect Button Input
static inline void button_event();                // Response Button Status
void controlAlarm(bool action);                              // Control Hardware Alarm      

// SENSORS
byte set_sensor();                                // Initialize Sensors
int  get_adc_average (byte ch);                   // Reading ADC-Channel Average
double get_thermocouple(void);                    // Reading Temperature KTYPE
void get_Vbat();                                   // Reading Battery Voltage
void cal_soc();

// TEMPERATURE (TEMP)
float calcT(int r, byte typ);                     // Calculate Temperature from ADC-Bytes
void get_Temperature();                           // Reading Temperature ADC
void set_Channels();                              // Initialize Temperature Channels
void transform_limits();                          // Transform Channel Limits

// OLED
#include <SSD1306.h>              
#include <OLEDDisplayUi.h>        
SSD1306 display(OLED_ADRESS, SDA, SCL);
OLEDDisplayUi ui     ( &display );

// FRAMES
void drawConnect();                       // Frane while System Start
void drawLoading();                               // Frame while Loading
void drawQuestion();                    // Frame while Question
void drawMenu();
void set_OLED();                                  // Configuration OLEDDisplay

// FILESYSTEM (FS)
bool loadfile(const char* filename, File& configFile);
bool savefile(const char* filename, File& configFile);
bool checkjson(JsonVariant json, const char* filename);
bool loadconfig(byte count);
bool setconfig(byte count, const char* data[2]);
bool modifyconfig(byte count, const char* data[12]);
void start_fs();                                  // Initialize FileSystem
void read_serial(char *buffer);                   // React to Serial Input 
int readline(int readch, char *buffer, int len);  // Put together Serial Input

// MEDIAN
void median_add(int value);                       // add Value to Buffer
void median_sort();                               // sort Buffer
double median_get();                              // get Median from Buffer

// OTA
void set_ota();                                   // Configuration OTA

// WIFI
void set_wifi();                                  // Connect WiFi
void get_rssi();
void reconnect_wifi();
void stop_wifi();
void check_wifi();

// SERVER
void buildDatajson(char *buffer, int len);
void buildSettingjson(char *buffer, int len);

// EEPROM
void setEE();
void writeEE(const char* json, int len, int startP);
void readEE(char *buffer, int len, int startP);
void clearEE(int startP, int endP);

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

bool isEco = false;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Response Button Status
static inline void button_event() {

  static unsigned long lastMupiTime;    // Zeitpunkt letztes schnelles Zeppen
  
  // Frage wurde mit YES bestätigt
  if ((buttonResult[0]==SHORTCLICK) && question > 0) {
      
      switch (question) {
        case CONFIGRESET:
          setconfig(eCHANNEL,{});
          loadconfig(eCHANNEL);
          set_Channels();
          break;

        case CHANGEUNIT:
          if (temp_unit == "C") temp_unit = "F";          // Change Unit
          else temp_unit = "C";
          transform_limits();                             // Transform Limits
          modifyconfig(eCHANNEL,{});                   // Save Config
          get_Temperature();                              // Update Temperature
          #ifdef DEBUG
            Serial.println("[INFO]\tEinheitenwechsel");
          #endif
          break;

        case HARDWAREALARM:
          controlAlarm(0);
          break;

      }
      question = NO;
      displayblocked = false;
      return;
  }

  // Frage wurde verneint -> alles bleibt beim Alten
  if ((buttonResult[1]==SHORTCLICK) && question > 0) {
      question = NO;
      displayblocked = false;
      return;
  }

  // Reset der Config erwuenscht
  if (buttonResult[1]==LONGCLICK && ui.getCurrentFrameCount()==0) {
      displayblocked = true;
      question = CONFIGRESET;
      drawQuestion();
      return;
  }

  // Button 1 Doppelclick während man sich im Hauptmenu befindet
  // -> Anzeigemodus wechseln
  if (buttonResult[0]==DOUBLECLICK && ui.getCurrentFrameCount()==0) {
    INACTIVESHOW = !INACTIVESHOW;

    #ifdef DEBUG
      Serial.println("[INFO]\tAnzeigewechsel");
    #endif
    return;
  }

  // Button 2 gedrückt während Menu -> Menupunkt bestätigen
  if (buttonResult[1]==SHORTCLICK && inMenu == MAINMENU) {

    displayblocked = false;
    switch (menu_count) {
      
      case 0:
        b_counter = 0;
        inMenu = TEMPSUB;
        break;

      case 1:
        b_counter = 5;
        inMenu = PITSUB;
        break;

      case 2:
        b_counter = 9;
        inMenu = SYSTEMSUB;
        break;
    }

    ui.switchToFrame(b_counter);
    return;
  }

  // Button 1 gedrückt während Menu -> Menupunkte durchgehen
  if (buttonResult[0]==SHORTCLICK && inMenu == MAINMENU) {
    if (menu_count < 2) menu_count++;
    else menu_count = 0;
    drawMenu();
    return;
  }
  
  // Button 1 Longclick während man sich im Hauptmenu befindet
  if (buttonResult[0]==LONGCLICK && ui.getCurrentFrameCount()==0) {
    // Falls Pitmaster nicht aktiv -> erstmal aktivieren
    // Code fehlt noch
    displayblocked = true;
    drawMenu();
    inMenu = MAINMENU;

    /*
    if (isEco) {
      reconnect_wifi();
      isEco = false;
    } else {
      stop_wifi();
      isEco = true;  
    }
    */
    
    return;
  }

  /*
  // Button 2 Doppelclick während man sich im Hauptmenu befindet
  // -> Einheit wechseln
  if (buttonResult[1]==DOUBLECLICK && ui.getCurrentFrameCount()==0) {
    displayblocked = true;
    question = CHANGEUNIT;
    drawQuestion();
    return;
  }
  */
  
  
  // Button 1 gedrückt während man sich im Hauptmenü befindet
  // -> Zum nächsten (aktiven) Kanal switchen
  if (buttonResult[0]==SHORTCLICK && ui.getCurrentFrameCount()==0) {

    b_counter = 0;
    int i = 0;          // Endlosschleife verhindern

    if (INACTIVESHOW) {
      current_ch++;
      if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
    }
    else {
      do {
        current_ch++;
        i++;
        if (current_ch > MAXCOUNTER) current_ch = MINCOUNTER;
      }
      while ((ch[current_ch].temp==INACTIVEVALUE) && (i<CHANNELS)); 
    }

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
          if (tempor > 200.0) tempor = 50.0;
          ch[current_ch].max = tempor;
          break;
        case 2:  // Lower Limit
          tempor = ch[current_ch].min +(0.1*mupi);
          if (tempor > 150.0) tempor = 10.0;
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
        case 5:  // Pitmaster Typ
          if (pitmaster.typ < pidsize-1) pitmaster.typ++;
          else pitmaster.typ = 0;
          break;
        case 6:  // Pitmaster Channel
          if (pitmaster.channel < CHANNELS-1) pitmaster.channel++;
          else pitmaster.channel = 0;
          break;
        case 7:  // Pitmaster Set
          tempor = pitmaster.set +(0.1*mupi);
          if (tempor > ch[pitmaster.channel].max) tempor = ch[pitmaster.channel].min;
          pitmaster.set = tempor;
          break;
        case 8:  // Pitmaster Active
          pitmaster.active = !pitmaster.active;
          break;
        case 12:  // Unit Change
          if (temp_unit == "C") temp_unit = "F";          // Change Unit
          else temp_unit = "C";
          transform_limits();                             // Transform Limits
          modifyconfig(eCHANNEL,{});                   // Save Config
          get_Temperature();                              // Update Temperature
          #ifdef DEBUG
            Serial.println("[INFO]\tEinheitenwechsel");
          #endif
          break;
        case 13:  // Hardware Alarm
          doAlarm = !doAlarm;
         
        break;
          
        default:
          break; 
      }
    }
  }
  
  // Button 2 gedrückt: -> Durchs Kontextmenu bewegen
  if (buttonResult[1]==SHORTCLICK) {
    
    b_counter = ui.getCurrentFrameCount();

    switch (inMenu) {

      case TEMPSUB:
        if (b_counter < 4) b_counter++;
        else {
          b_counter = 0;
          modifyconfig(eCHANNEL,{});             // Am Ende des Kontextmenu Config speichern
        }
        ui.switchToFrame(b_counter);
        break;

      case PITSUB:
        if (b_counter < 8) {
          b_counter++;
          ui.switchToFrame(b_counter);
        }
        else {
          displayblocked = true;
          drawMenu();
          inMenu = MAINMENU;
        }
        break;

      case SYSTEMSUB:
        if (b_counter < 14) {
          b_counter++;
          ui.switchToFrame(b_counter);
        }
        else {
          displayblocked = true;
          drawMenu();
          inMenu = MAINMENU;
        }
        break;
    
    }
    
  }

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}






