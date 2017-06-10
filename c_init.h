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
    
    HISTORY: Please refer Github History
    
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
#include <ESP8266mDNS.h>        
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/60
#include "AsyncJson.h"

#include <StreamString.h>

extern "C" {
#include "user_interface.h"
#include "spi_flash.h"
#include "core_esp8266_si2c.c"
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
#define SENSORTYPEN    8               // NUMBER OF SENSORS
#define LIMITUNTERGRENZE -20           // MINIMUM LIMIT
#define LIMITOBERGRENZE 999            // MAXIMUM LIMIT
#define MAX1161x_ADDRESS 0x33          // MAX11615
#define ULIMITMIN 10.0
#define ULIMITMAX 150.0
#define OLIMITMIN 35.0
#define OLIMITMAX 200.0
#define ULIMITMINF 50.0
#define ULIMITMAXF 302.0
#define OLIMITMINF 95.0
#define OLIMITMAXF 392.0

// BATTERY
#define BATTMIN 3600                  // MINIMUM BATTERY VOLTAGE in mV
#define BATTMAX 4185                  // MAXIMUM BATTERY VOLTAGE in mV 
#define ANALOGREADBATTPIN 0           // INTERNAL ADC PIN
#define BATTDIV 5.9F
#define CHARGEDETECTION 16              // LOAD DETECTION PIN

// OLED
#define OLED_ADRESS 0x3C              // OLED I2C ADRESS
#define MAXBATTERYBAR 13

// TIMER
#define INTERVALBATTERYMODE 1000
#define INTERVALSENSOR 1000
#define INTERVALCOMMUNICATION 30000
#define FLASHINWORK 500

// BUS
#define SDA 0
#define SCL 2
#define THERMOCOUPLE_CS 12          // 12

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
#define HOSTNAME "NANO-"
#define NTP_PACKET_SIZE 48          // NTP time stamp is in the first 48 bytes of the message

// FILESYSTEM
#define CHANNELJSONVERSION 4        // FS VERSION
#define EEPROM_SIZE 1920            // EEPROM SIZE
#define EEWIFIBEGIN         0
#define EEWIFI              300
#define EESYSTEMBEGIN       EEWIFIBEGIN+EEWIFI
#define EESYSTEM            250
#define EECHANNELBEGIN      EESYSTEMBEGIN+EESYSTEM
#define EECHANNEL           500
#define EETHINGBEGIN        EECHANNELBEGIN+EECHANNEL
#define EETHING             150
#define EEPITMASTERBEGIN    EETHINGBEGIN+EETHING
#define EEPITMASTER         700

// PITMASTER
#define PITMASTER1 15               // PITMASTER PIN
#define PITMASTER2 14             // ab Platine V7.2
#define PITMIN 0                    // LOWER LIMIT SET
#define PITMAX 100                  // UPPER LIMIT SET
#define PITMASTERSIZE 5             // PITMASTER SETTINGS LIMIT

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// GLOBAL VARIABLES

// CHANNELS
struct ChannelData {
   String name;             // CHANNEL NAME
   float temp;              // TEMPERATURE
   int   match;             // Anzeige im Temperatursymbol
   float max;               // MAXIMUM TEMPERATURE
   float min;               // MINIMUM TEMPERATURE
   byte  typ;               // TEMPERATURE SENSOR
   bool  alarm;             // SET CHANNEL ALARM
   bool  isalarm;           // Limits überschritten
   bool  show;              // Anzeigen am OLED       
   bool  showalarm;         // Alarm nicht weiter anzeigen
   String color;            // COLOR
};

ChannelData ch[CHANNELS];

String  ttypname[SENSORTYPEN] = {"Maverick",
                      "Fantast-Neu",
                      "100K6A1B",
                      "100K",
                      "SMD NTC",
                      "5K3A1B",
                      "MOUSER47K",
                      "Typ K"};


String  temp_unit = "C";
//String colors[8] = {"#6495ED", "#CD2626", "#66CDAA", "#F4A460", "#D02090", "#FFEC8B", "#BA55D3", "#008B8B"};
String colors[8] = {"#0C4C88","#22B14C","#EF562D","#FFC100","#A349A4","#804000","#5587A2","#5C7148"};

// PITMASTER
struct Pitmaster {
   byte pid;           // PITMASTER PID-Setting
   float set;            // SET-TEMPERATUR
   bool  active;           // IS PITMASTER ACTIVE
   byte  channel;         // PITMASTER CHANNEL
   float value;           // PITMASTER VALUE IN %
   int manuel;            // MANUEL PITMASTER VALUE IN %
   bool event;
   int16_t msec;          // PITMASTER VALUE IN MILLISEC
   unsigned long last;
   int pause;             // PITMASTER PAUSE
};

Pitmaster pitmaster;
int pidsize;


// DATALOGGER
struct datalogger {
 uint16_t tem[8];
 long timestamp;
 uint8_t pitmaster;
 uint8_t soll;
};

#define MAXLOGCOUNT 155             // SPI_FLASH_SEC_SIZE/ sizeof(datalogger)
datalogger mylog[MAXLOGCOUNT];
datalogger archivlog[MAXLOGCOUNT];
unsigned long log_count = 0;
uint32_t log_sector;                // erster Sector von APP2
uint32_t freeSpaceStart;            // First Sector of OTA
uint32_t freeSpaceEnd;              // Last Sector+1 of OTA


// SYSTEM
struct System {
   byte hwversion;           // HARDWARE VERSION
   bool fastmode;              // FAST DISPLAY MODE
   String apname;             // AP NAME
   bool summer;              // SUMMER TIME
   String host;                     // HOST NAME
   String language;           // SYSTEM LANGUAGE
   int timeZone;              // TIMEZONE
   bool hwalarm;              // HARDWARE ALARM 
};

System sys;
bool stby = false;                // USB POWER SUPPLY?            
byte pulsalarm = 1;


// BATTERY
struct Battery {
  int voltage;                    // CURRENT VOLTAGE
  bool charge;                    // CHARGE DETECTION
  int percentage;                 // BATTERY CHARGE STATE in %
  bool setreference;              // LOAD COMPLETE SAVE VOLTAGE
  int max;                        // MAX VOLTAGE
  int min;
};

Battery battery;
uint32_t vol_sum = 0;
int vol_count = 0;

// CHARTS
struct Charts {
   String TSwriteKey;           // THINGSPEAK WRITE API KEY
   String TShttpKey;            // THINGSPEAK HTTP API KEY 
   String TSchID;                // THINGSPEAK CHANNEL ID 
   bool TSshow8;
};

Charts charts;

// OLED
int current_ch = 0;               // CURRENTLY DISPLAYED CHANNEL       
bool LADENSHOW = false;           // LOADING INFORMATION?
bool displayblocked = false;                     // No OLED Update
enum {NO, CONFIGRESET, CHANGEUNIT, OTAUPDATE, HARDWAREALARM};

struct MyQuestion {
   int typ;    
   int con;            
};

MyQuestion question;

// FILESYSTEM
enum {eCHANNEL, eWIFI, eTHING, ePIT, eSYSTEM, ePRESET};

// WIFI
byte isAP = 2;                    // WIFI MODE  (0 = STA, 1 = AP, 2 = NO, 3 = Turn off)
String wifissid[5];
String wifipass[5];
int lenwifi = 0;
long rssi = 0;                   // Buffer rssi

long scantime;
bool disconnectAP;
struct HoldSSID {
   unsigned long connect;
   bool hold;             
   String ssid;
   String pass;
};
HoldSSID holdssid;

// NTP
byte packetBuffer[ NTP_PACKET_SIZE];    //buffer to hold incoming and outgoing packets

// BUTTONS
byte buttonPins[]={btn_r,btn_l};          // Pins
#define NUMBUTTONS sizeof(buttonPins)
byte buttonState[NUMBUTTONS];     // Aktueller Status des Buttons HIGH/LOW
enum {NONE, FIRSTDOWN, FIRSTUP, SHORTCLICK, DOUBLECLICK, LONGCLICK};
byte buttonResult[NUMBUTTONS];    // Aktueller Klickstatus der Buttons NONE/SHORTCLICK/LONGCLICK
unsigned long buttonDownTime[NUMBUTTONS]; // Zeitpunkt FIRSTDOWN
byte menu_count = 0;                      // Counter for Menu
byte inMenu = 0;
enum {TEMPSUB, PITSUB, SYSTEMSUB, MAINMENU, TEMPKONTEXT, BACK};
bool inWork = 0;
bool isback = 0;
int framepos[3] = {0, 6, 11};
int frameCount = 19;
bool flashinwork = true;
float tempor;                       // Zwischenspeichervariable

// TIMER
unsigned long lastUpdateBatteryMode;
unsigned long lastUpdateSensor;
unsigned long lastUpdatePiepser;
unsigned long lastUpdateCommunication;
unsigned long lastUpdateDatalog;
unsigned long lastFlashInWork;
unsigned long lastUpdateRSSI;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// PRE-DECLARATION

// INIT
void set_serial();                                // Initialize Serial
void set_button();                                // Initialize Buttons
static inline boolean button_input();             // Dectect Button Input
static inline void button_event();                // Response Button Status
void controlAlarm(bool action);                              // Control Hardware Alarm
void set_piepser();
void piepserOFF();
void piepserON();      

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
void drawQuestion(int counter);                    // Frame while Question
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
void write_flash(uint32_t _sector);

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
time_t getNtpTime();

// SERVER
void handleSettings(AsyncWebServerRequest *request, bool www);
void handleData(AsyncWebServerRequest *request, bool www);
void handleWifiResult(AsyncWebServerRequest *request, bool www);
void handleWifiScan(AsyncWebServerRequest *request, bool www);
bool handleSetNetwork(AsyncWebServerRequest *request, uint8_t *datas);
bool handleSetSystem(AsyncWebServerRequest *request, uint8_t *datas);
bool handleSetChart(AsyncWebServerRequest *request, uint8_t *datas);
bool handleSetPitmaster(AsyncWebServerRequest *request, uint8_t *datas);
bool handleSetChannels(AsyncWebServerRequest *request, uint8_t *datas);
bool handleAddPitmaster(AsyncWebServerRequest *request, uint8_t *datas);

// EEPROM
void setEE();
void writeEE(const char* json, int len, int startP);
void readEE(char *buffer, int len, int startP);
void clearEE(int startP, int endP);

// PITMASTER
void startautotunePID(int maxCycles, bool storeValues);
void pitmaster_control();

// BOT
#ifdef THINGSPEAK
void sendMessage(int ch, int count);
void sendData();
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Serial
void set_serial() {
  Serial.begin(115200);
  DPRINTLN();
  DPRINTLN();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Temperature and Battery Measurement Timer
void timer_sensor() {
  
  if (millis() - lastUpdateSensor > INTERVALSENSOR) {
    get_Temperature();
    get_Vbat();
    lastUpdateSensor = millis();
  }

  if (millis() - lastUpdateRSSI > INTERVALCOMMUNICATION) {
    get_rssi(); 
    cal_soc();
    lastUpdateRSSI = millis();
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Temperature Alarm Timer
void timer_alarm() {
  
  if (millis() - lastUpdatePiepser > INTERVALSENSOR/4) {
    controlAlarm(pulsalarm);
    pulsalarm = !pulsalarm;
    lastUpdatePiepser = millis();
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Charts Timer
void timer_charts() {
  
  if (millis() - lastUpdateCommunication > INTERVALCOMMUNICATION) {

    if (!isAP) {
      #ifdef THINGSPEAK
        if (charts.TSwriteKey != "") sendData();
      #endif
    }
    lastUpdateCommunication = millis();
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// DataLog Timer
void timer_datalog() {  
  
  if (millis() - lastUpdateDatalog > 60000) {

    //Serial.println(sizeof(datalogger));
    //Serial.println(sizeof(mylog));

    int logc;
    if (log_count < MAXLOGCOUNT) logc = log_count;
    else {
      logc = MAXLOGCOUNT-1;
      memcpy(&mylog[0], &mylog[1], (MAXLOGCOUNT-1)*sizeof(*mylog));
    }

    for (int i=0; i < CHANNELS; i++)  {
      mylog[logc].tem[i] = (uint16_t) (ch[i].temp * 10);       // 8 * 16 bit  // 8 * 2 byte
    }
    mylog[logc].pitmaster = (uint8_t) pitmaster.value;    // 8 bit  // 1 byte
    mylog[logc].soll = (uint8_t) pitmaster.set;           // 8 bit  // 1 byte
    mylog[logc].timestamp = now();     // 64 bit // 8 byte

    log_count++;
    // 2*8 + 2 + 8 = 26
    if (log_count%MAXLOGCOUNT == 0 && log_count != 0) {
        
      if (log_sector > freeSpaceEnd/SPI_FLASH_SEC_SIZE) 
        log_sector = freeSpaceStart/SPI_FLASH_SEC_SIZE;
        
      write_flash(log_sector);
      log_sector++;
      modifyconfig(eSYSTEM,{});

        //getLog(3);
        
    }
    lastUpdateDatalog = millis();
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Flash
void flash_control() { 
  if (inWork) {
    if (millis() - lastFlashInWork > FLASHINWORK) {
      flashinwork = !flashinwork;
      lastFlashInWork = millis();
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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Show time
String printDigits(int digits){
  String com;
  if(digits < 10) com = "0";
  com += String(digits);
  return com;
}

String digitalClockDisplay(time_t t){

  String zeit;
  zeit += printDigits(hour(t))+":";
  zeit += printDigits(minute(t))+":";
  zeit += printDigits(second(t))+" ";
  zeit += String(day(t))+".";
  zeit += String(month(t))+".";
  zeit += String(year(t));
  return zeit;
}


String newDate(time_t t){

  String zeit;
  zeit += "new Date(";
  zeit += String(year(t))+",";
  zeit += String(month(t)-1)+",";
  zeit += String(day(t))+",";
  zeit += String(hour(t))+",";
  zeit += String(minute(t))+",";
  zeit += String(second(t))+")";
  
  return zeit;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SYSTEM TIME based on UTC
time_t mynow() {

  if (sys.summer) return now() + (sys.timeZone+1) * SECS_PER_HOUR;
  else return now() + sys.timeZone * SECS_PER_HOUR;
  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Update Time
void set_time() {
  if (!isAP) {
    time_t present = 0;
    int ii = 0;
    while (present == 0 && ii < 3) {
      present = getNtpTime(); 
      ii++;
    }
    setTime(present);
  }
  //setSyncProvider(getNtpTime);
  DPRINTP("[INFO]\t");
  DPRINTLN(digitalClockDisplay(mynow()));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Standby oder Mess-Betrieb
bool standby_control() {
  if (stby) {

    drawLoading();
    if (!LADENSHOW) {
      //drawLoading();
      LADENSHOW = true;
      DPRINTPLN("[INFO]\tChange to Standby");
      //stop_wifi();  // führt warum auch immer bei manchen Nanos zu ständigem Restart
      pitmaster.active = false;
      piepserOFF();
      // set_pitmaster();
    }
    
    if (millis() - lastUpdateBatteryMode > INTERVALBATTERYMODE) {
      get_Vbat();
      lastUpdateBatteryMode = millis();  

      if (!stby) ESP.restart();
    }
    
    return 1;
  }
  return 0;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Nachkommastellen limitieren
float limit_float(float f, int i) {
  if (ch[i].temp!=INACTIVEVALUE) {
    f = f + 0.05;                   // damit er "richtig" rundet, bei 2 nachkommastellen 0.005 usw.
    f = (int)(f*10);               // hier wird der float *10 gerechnet und auf int gecastet, so fallen alle weiteren Nachkommastellen weg
    f = f/10;
  } else f = 999;
  return f;
}






