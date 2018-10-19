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
//#include <SPI.h>                  // SPI
//#include <ESP8266WiFi.h>          // WIFI
//#include <WiFiClientSecure.h>     // HTTPS
#include <TimeLib.h>              // TIME
#include <EEPROM.h>               // EEPROM
#include <FS.h>                   // FILESYSTEM
#include <ArduinoJson.h>          // JSON
#include <ESP8266mDNS.h>          // mDNS
#include <ESPAsyncTCP.h>          // ASYNCTCP
#include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/60
#include "AsyncJson.h"            // ASYNCJSON
#include <AsyncMqttClient.h>      // ASYNCMQTT
//#include <StreamString.h>

extern "C" {
#include "user_interface.h"
#include "spi_flash.h"
#include "core_esp8266_si2c.c"
}

extern "C" uint32_t _SPIFFS_start;      // START ADRESS FS
extern "C" uint32_t _SPIFFS_end;        // FIRST ADRESS AFTER FS

// ++++++++++++++++++++++++++++++++++++++++++++++++++
// Nano V2: MISO > Supply Switch; CLK > PIT2

// ++++++++++++++++++++++++++++++++++++++++++++++++++
// SETTINGS

// HARDWARE
#define FIRMWAREVERSION "v0.9.13"
#define APIVERSION      "2"

// CHANNELS
#define CHANNELS 8                     // UPDATE AUF HARDWARE 4.05
#define INACTIVEVALUE  999             // NO NTC CONNECTED
#define SENSORTYPEN    11               // NUMBER OF SENSORS
#define LIMITUNTERGRENZE -31           // MINIMUM LIMIT
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
#define BATTMIN 3550                  // MINIMUM BATTERY VOLTAGE in mV
#define BATTMAX 4170                  // MAXIMUM BATTERY VOLTAGE in mV 
#define ANALOGREADBATTPIN 0           // INTERNAL ADC PIN
#define BATTDIV 5.9F
#define CHARGEDETECTION 16              // LOAD DETECTION PIN
#define CORRECTIONTIME 60000
#define BATTERYSTARTUP 10000

// OLED
#define OLED_ADRESS 0x3C              // OLED I2C ADRESS
#define MAXBATTERYBAR 13

// TIMER (bezogen auf 250 ms Timer) (x mal 250)
#define INTERVALSENSOR 4              // 1 s
#define INTERVALCOMMUNICATION 120     // 30 s
#define INTERVALBATTERYSIM 120        // 30 s
#define FLASHINWORK 2                 // 500 ms

// BUS
#define SDA 0
#define SCL 2
#define THERMOCOUPLE_CS 12          // Nur Test-Versionen, Konflikt Pitsupply

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

// FILESYSTEM
#define CHANNELJSONVERSION 4        // FS VERSION
#define EEPROM_SIZE 2816            // EEPROM SIZE
#define EEWIFIBEGIN         0
#define EEWIFI              300
#define EESYSTEMBEGIN       EEWIFIBEGIN+EEWIFI
#define EESYSTEM            380
#define EECHANNELBEGIN      EESYSTEMBEGIN+EESYSTEM
#define EECHANNEL           500
#define EETHINGBEGIN        EECHANNELBEGIN+EECHANNEL
#define EETHING             420
#define EEPITMASTERBEGIN    EETHINGBEGIN+EETHING
#define EEPITMASTER         700
#define EEPUSHBEGIN         EEPITMASTERBEGIN+EEPITMASTER
#define EEPUSH              512         

// PITMASTER
#define PITMASTER1 15               // PITMASTER PIN
#define PITMASTER2 14               // CLK // ab Platine V7.2
#define PITSUPPLY  12               // MISO // ab Platine V9.3
#define PITMIN 0                    // LOWER LIMIT SET
#define PITMAX 100                  // UPPER LIMIT SET
#define PITMASTERSIZE 2             // PITMASTER SETTINGS LIMIT
#define PIDSIZE 3
#define PITMASTERSETMIN 50
#define PITMASTERSETMAX 200
#define SERVOPULSMIN 550  // 25 Grad    // 785
#define SERVOPULSMAX 2250

#define PRODUCTNUMBERLENGTH 11


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
   byte  alarm;             // SET CHANNEL ALARM (0: off, 1:push, 2:summer, 3:all)
   bool  isalarm;           // Limits überschritten   
   byte  showalarm;         // Alarm anzeigen   (0:off, 1:show, 2:first show)
   String color;            // COLOR
};

ChannelData ch[CHANNELS];

enum {ALARM_OFF,ALARM_PUSH,ALARM_HW,ALARM_ALL};
String alarmname[4] = {"off","push","summer","all"};

// SENSORTYP
String  ttypname[SENSORTYPEN] = {"Maverick","Fantast-Neu","Fantast","iGrill2","ET-73",
                                 "Perfektion","50K","Inkbird","100K6A1B","Weber_6743",
                                 "Santos"}; // 

// CHANNEL COLORS
String colors[8] = {"#0C4C88","#22B14C","#EF562D","#FFC100","#A349A4","#804000","#5587A2","#5C7148"};

// PITMASTER
enum {PITOFF, MANUAL, AUTO, AUTOTUNE, DUTYCYCLE, VOLTAGE};
enum {SSR, FAN, SERVO, DAMPER, SUPPLY};

struct Pitmaster {
   byte pid;              // PITMASTER PID-Setting
   float set;             // SET-TEMPERATUR
   byte active;           // IS PITMASTER ACTIVE
   byte  channel;         // PITMASTER CHANNEL
   float value;           // PITMASTER VALUE IN %
   uint16_t dcmin;        // PITMASTER DUTY CYCLE LIMIT MIN
   uint16_t dcmax;        // PITMASTER DUTY CYCLE LIMIT MIN
   byte io;               // PITMASTER HARDWARE IO
   bool event;            // SSR HIGH EVENT
   uint16_t msec;         // PITMASTER VALUE IN MILLISEC (SSR) / MICROSEC (SERVO)
   uint16_t nmsec;
   unsigned long stakt;   //
   unsigned long last;    // PITMASTER VALUE TIMER
   uint16_t pause;        // PITMASTER PAUSE
   bool resume;           // Continue after restart 
   long timer0;           // PITMASTER TIMER VARIABLE (FAN) / (SERVO)
   float esum;            // PITMASTER I-PART DIFFERENZ SUM
   float elast;           // PITMASTER D-PART DIFFERENZ LAST
   float Ki_alt;
   bool disabled;         // PITMASTER DISABLE HEATER
   bool pwm;              // PITMASTER USES PWM (FAN)    
};

Pitmaster pitMaster[PITMASTERSIZE];
int pidsize;

// PID PROFIL
struct PID {
  String name;
  byte id;
  byte aktor;                   // 0: SSR, 1:FAN, 2:Servo, 3:Damper
  float Kp;                     // P-FAKTOR ABOVE PSWITCH
  float Ki;                     // I-FAKTOR ABOVE PSWITCH
  float Kd;                     // D-FAKTOR ABOVE PSWITCH
  float Kp_a;                   // P-FAKTOR BELOW PSWITCH
  float Ki_a;                   // I-FAKTOR ABOVE PSWITCH
  float Kd_a;                   // D-FAKTOR ABOVE PSWITCH
  int Ki_min;                   // MINIMUM VALUE I-PART   // raus ?
  int Ki_max;                   // MAXIMUM VALUE I-PART   // raus ?
  float pswitch;                // SWITCHING LIMIT        // raus ?
  float DCmin;                  // PID DUTY CYCLE MIN
  float DCmax;                  // PID DUTY CYCLE MAX
  byte opl;
};
PID pid[PIDSIZE];

// AUTOTUNE
struct AutoTune {
   bool storeValues;
   float temp;             // BETRIEBS-TEMPERATUR
   int  maxCycles;        // WIEDERHOLUNGEN
   int cycles;            // CURRENT WIEDERHOLUNG
   int heating;            // HEATING FLAG
   uint32_t t0;
   uint32_t t1;            // ZEITKONSTANTE 1
   uint32_t t2;           // ZEITKONSTANTE 2
   int32_t t_high;        // FLAG HIGH
   int32_t t_low;         // FLAG LOW
   int32_t bias;
   int32_t d;
   float Kp;
   float Ki;
   float Kd;
   float Kp_a;
   float Ki_a;
   float Kd_a;
   float maxTemp;
   float minTemp;
   bool initialized;
   float value;
   float pTemp;
   float maxTP;             // MAXIMALE STEIGUNG = WENDEPUNKT
   uint32_t tWP;            // ZEITPUNKT WENDEPUNKT  
   float TWP;               // TEMPERATUR WENDEPUNKT
   bool start;
   byte stop;
   int overtemp;
   long timelimit;
   bool keepup;             // PITMASTER FORTSETZEN NACH ENDE
};

AutoTune autotune;

// DUTYCYCLE TEST
struct DutyCycle {
  long timer;
  int value;        // Value * 10
  bool dc;          // min or max
  byte aktor;
  int saved;
};

DutyCycle dutyCycle[PITMASTERSIZE];


uint32_t log_sector;                // erster Sector von APP2
uint32_t freeSpaceStart;            // First Sector of OTA
uint32_t freeSpaceEnd;              // Last Sector+1 of OTA

// NOTIFICATION
struct Notification {
  byte index;                       // INDEX BIN
  byte ch;                          // CHANNEL BIN
  byte limit;                       // LIMIT: 0 = LOW TEMPERATURE, 1 = HIGH TEMPERATURE
  byte type;                        // TYPE: 0 = NORMAL MODE, 1 = TEST MESSAGE
};

Notification notification;

// SYSTEM
struct System {
   String unit = "C";         // TEMPERATURE UNIT
   byte hwversion;           // HARDWARE VERSION
   bool fastmode;              // FAST DISPLAY MODE
   String apname;             // AP NAME
   String host;                     // HOST NAME
   String language;           // SYSTEM LANGUAGE
   byte updatecount;           // 
   int update;             // FIRMWARE UPDATE -1 = check, 0 = no, 1 = spiffs, 2 = firmware
   String getupdate;
   bool autoupdate;
   byte god;
   bool pitsupply;        
   bool stby;                   // STANDBY
   bool restartnow; 
   bool typk;
   bool damper;
   bool sendSettingsflag;          // SENDSETTINGS FLAG
   const char* www_username = "admin";
   String www_password = "admin";
   bool advanced;
   String item;
   //char item[8];
   //bool nobattery;
};

System sys;

byte pulsalarm = 1;

// BATTERY
struct Battery {
  int voltage;                    // CURRENT VOLTAGE
  bool charge;                    // CHARGE DETECTION
  int percentage;                 // BATTERY CHARGE STATE in %
  int setreference;              // LOAD COMPLETE SAVE VOLTAGE
  int max;                        // MAX VOLTAGE
  int min;                        // MIN VOLTAGE
  int correction = 0;   
  byte state;                   // 0:LOAD, 1:SHUTDOWN,  3:COMPLETE
  int sim;                        // SIMULATION VOLTAGE
  byte simc;                      // SIMULATION COUNTER
};

Battery battery;
uint32_t vol_sum = 0;
int vol_count = 0;


// IOT
struct IoT {
   String TS_writeKey;          // THINGSPEAK WRITE API KEY
   String TS_httpKey;           // THINGSPEAK HTTP API KEY 
   String TS_userKey;           // THINGSPEAK USER KEY 
   String TS_chID;              // THINGSPEAK CHANNEL ID 
   bool TS_show8;               // THINGSPEAK SHOW SOC
   int TS_int;                  // THINGSPEAK INTERVAL IN SEC
   bool TS_on;                  // THINGSPEAK ON / OFF
   String P_MQTT_HOST;          // PRIVATE MQTT BROKER HOST
   uint16_t P_MQTT_PORT;        // PRIVATE MQTT BROKER PORT
   String P_MQTT_USER;          // PRIVATE MQTT BROKER USER
   String P_MQTT_PASS;          // PRIVATE MQTT BROKER PASSWD
   byte P_MQTT_QoS;             // PRIVATE MQTT BROKER QoS
   bool P_MQTT_on;              // PRIVATE MQTT BROKER ON/OFF
   int P_MQTT_int;              // PRIVATE MQTT BROKER IN SEC 
   bool CL_on;                  // NANO CLOUD ON / OFF
   String CL_token;             // NANO CLOUD TOKEN
   int CL_int;                  // NANO CLOUD INTERVALL
};

IoT iot;

bool lastUpdateCloud;


struct PushD {
   byte on;                  // NOTIFICATION SERVICE OFF(0)/ON(1)/TEST(2)/CLEAR(3)
   String token;             // API TOKEN
   String id;                // CHAT ID 
   int repeat;               // REPEAT PUSH NOTIFICATION
   byte service;             // SERVICE
  
};

PushD pushd; 

// CLOUD CHART/LOG
struct Chart {
   bool on = false;                  // NANO CHART ON / OFF
};

Chart chart;

// OLED
int current_ch = 0;               // CURRENTLY DISPLAYED CHANNEL     
bool ladenshow = false;           // LOADING INFORMATION?
bool displayblocked = false;                     // No OLED Update
enum {NO, CONFIGRESET, CHANGEUNIT, OTAUPDATE, HARDWAREALARM, IPADRESSE, TUNE, SYSTEMSTART, RESETWIFI, RESETFW};

// OLED QUESTION
struct MyQuestion {
   int typ;    
   int con;            
};

MyQuestion question;

// FILESYSTEM
enum {eCHANNEL, eWIFI, eTHING, ePIT, eSYSTEM, ePUSH, ePRESET};


struct OpenLid {
   bool detected;         // Open Lid Detected
   float ref[5] = {0.0, 0.0, 0.0, 0.0, 0.0};          // Open Lid Temperatur Memory
   float temp;            // Temperatur by Open Lid
   int  count;            // Open Lid Count
};

OpenLid opl;



// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h

// WIFI
struct Wifi {
  byte mode;                       // WIFI MODE  (0 = OFF, 1 = STA, 2 = AP, 3/4 = Turn off), 5 = DISCONNECT, 6 = CONNECTING, 7 = OPEN
  unsigned long turnoffAPtimer;    // TURN OFF AP TIMER
  byte savedlen;                   // LENGTH SAVED WIFI DATE
  String savedssid[5];             // SAVED SSID
  String savedpass[5];             // SAVED PASSWORD
  int rssi;                        // BUFFER RSSI
  byte savecount;                  // COUNTER
  unsigned long reconnecttime;
  unsigned long scantime;          // LAST SCAN TIME
  bool disconnectAP;               // DISCONNECT AP
  bool revive;
  bool takeAP;
  long timerAP;
  byte neu;                         // SAVE-MODE: (0 = no save, 1 = only sort, 2 = add new)
  unsigned long mqttreconnect;
};
Wifi wifi;

struct HoldSSID {
   unsigned long connect;           // NEW WIFI DATA TIMER  (-1: in Process)
   byte hold;                       // NEW WIFI DATA      
   String ssid;                     // NEW SSID
   String pass;                     // NEW PASSWORD
};
HoldSSID holdssid;

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
byte framepos[5] = {0, 2, 3, 1, 4};  // TempSub, PitSub, SysSub, TempKon, Back
byte subframepos[4] = {1, 6, 11, 17};    // immer ein Back dazwischen // menutextde ebenfalls anpassen
int current_frame = 0;  
bool flashinwork = true;
float tempor;                       // Zwischenspeichervariable

// WEBSERVER
AsyncWebServer server(80);        // https://github.com/me-no-dev/ESPAsyncWebServer

// TIMER
unsigned long lastUpdateBatteryMode;


rst_info *myResetInfo;

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
void set_sensor();                                // Initialize Sensors
int  get_adc_average (byte ch);                   // Reading ADC-Channel Average
void get_Vbat();                                   // Reading Battery Voltage
void cal_soc();
void battery_set_full(bool full);
void battery_reset_reference();

// TEMPERATURE (TEMP)
float calcT(int r, byte typ);                     // Calculate Temperature from ADC-Bytes
void get_Temperature();                           // Reading Temperature ADC
void set_channels(bool init);                              // Initialize Temperature Channels
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
bool loadconfig(byte count, bool old);
bool setconfig(byte count, const char* data[2]);
bool modifyconfig(byte count, bool neu);
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
void check_http_update();

// WIFI
void set_wifi();                                  // Connect WiFi
void get_rssi();
void reconnect_wifi();
void stop_wifi();
void check_wifi();
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDHCPTimeout, wifiDisconnectHandler, softAPDisconnectHandler;  
void connectToMqtt();
void EraseWiFiFlash();
void connectWiFi();

//MQTT
AsyncMqttClient pmqttClient;
bool sendpmqtt();
bool sendSettings();

// EEPROM
void setEE();
void writeEE(const char* json, int len, int startP);
void readEE(char *buffer, int len, int startP);
void clearEE(int startP, int endP);

// PITMASTER
void startautotunePID(int maxCyc, bool store, int over, long tlimit, byte id);
void pitmaster_control(byte id);
void disableAllHeater();
void disableHeater(byte id, bool hold = false);
void set_pitmaster(bool init);
void set_pid(byte index);
void stopautotune(byte id);
void DC_start(bool dc, byte aktor, int val, byte id);
void open_lid();
void open_lid_init();

// BOT
void set_iot(bool init);
void set_push();
String collectData();
String createNote(bool ts);
bool sendNote(int check);
void sendDataTS();

void sendServerLog();
String serverLog();
void sendDataCloud();

//String cloudData(bool cloud);
String cloudData(bool cloud, bool get_sys = true, uint8_t get_ch = CHANNELS, uint8_t get_pit = 1);
//String cloudSettings();
String cloudSettings(bool get_sys = true, bool get_sen = true, uint8_t get_pid = pidsize, bool get_akt = true, bool get_iot = true, bool get_not = true);

String connectionStatus ( int which );

//void setWebSocket();

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Serial
void set_serial() {
  Serial.begin(115200);
  DPRINTLN();
  DPRINTLN();
  IPRINTLN(ESP.getResetReason());

  myResetInfo = ESP.getResetInfoPtr();
  //Serial.printf("myResetInfo->reason %x \n", myResetInfo->reason); // reason is uint32
  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check why reset
bool checkResetInfo() {

  // Source: Arduino/cores/esp8266/Esp.cpp
  // Source: Arduino/tools/sdk/include/user_interface.h

  switch (myResetInfo->reason) {

    case REASON_DEFAULT_RST: 
    case REASON_SOFT_RESTART:       // SOFTWARE RESTART
    case REASON_EXT_SYS_RST:          // EXTERNAL (FLASH)
    case REASON_DEEP_SLEEP_AWAKE:     // WAKE UP
      return true;  

    case REASON_EXCEPTION_RST:      // EXEPTION
    case REASON_WDT_RST:            // HARDWARE WDT
    case REASON_SOFT_WDT_RST:       // SOFTWARE WDT
      break;
  }

  return false;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize System-Settings, if not loaded from EE
void set_system() {
  
  String host = HOSTNAME;
  host += String(ESP.getChipId(), HEX);
  
  sys.host = host;
  sys.apname = APNAME;
  sys.language = "de";
  sys.fastmode = false;
  sys.hwversion = 1;
  if (sys.update == 0) sys.getupdate = "false";   // Änderungen am EE während Update
  sys.autoupdate = 1;
  sys.god = false;
  sys.typk = false;
  battery.max = BATTMAX;
  battery.min = BATTMIN;
  battery.setreference = 0;
  sys.pitsupply = false;           // nur mit Mod
  sys.damper = false;
  sys.restartnow = false;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main Timer

os_timer_t Timer1;         
bool osticker = false;
uint16_t oscounter = 0;

void timerCallback(void *pArg) { 
  osticker = true;
  *((int *) pArg) += 1;
} 

void set_ostimer() {
 os_timer_setfn(&Timer1, timerCallback, &oscounter);
 os_timer_arm(&Timer1, 250, true);
}

void maintimer(bool stby = false) {
  if (osticker) { 

    if (stby) {
      if (!(oscounter % 4)) {     // 1 s
        get_Vbat(); 
        if (!sys.stby) ESP.restart();
      }
    } else {

      // Temperature and Battery Measurement Timer
      if (!(oscounter % INTERVALSENSOR)) {     // 1 s
        get_Temperature();                            // Temperature Measurement
        get_Vbat();                                   // Battery Measurement
        if (millis() < BATTERYSTARTUP) cal_soc();     // schnelles aktualisieren beim Systemstart
      }

      // RSSI and kumulative Battery Measurement
      if (!(oscounter % INTERVALBATTERYSIM)) {     // 30 s
        get_rssi();                                   // RSSI Measurement 
        cal_soc();                                    // Kumulative Battery Value
      }

      // Alarm Puls Timer
      if (!(oscounter % 1)) {       // 250 ms
        controlAlarm(pulsalarm);
        pulsalarm = !pulsalarm;
      }

      // THINGSPEAK
      if (!(oscounter % iot.TS_int*4)) {       // variable
        if (wifi.mode == 1 && sys.update == 0 && iot.TS_on) {
          if (iot.TS_writeKey != "" && iot.TS_chID != "") sendDataTS();
        }
      }

      // PRIVATE MQTT
      if (!(oscounter % iot.P_MQTT_int*4)) {   // variable
        if (wifi.mode == 1 && sys.update == 0 && iot.P_MQTT_on) sendpmqtt();
      } 

      // NANO CLOUD
      if (!(oscounter % (iot.CL_int*4)) || lastUpdateCloud) {   // variable
        if (wifi.mode == 1 && sys.update == 0 && iot.CL_on) sendDataCloud();
        lastUpdateCloud = false;
      }

      // OLED FLASH TIMER
      if (inWork) {
        if (!(oscounter % FLASHINWORK)) {     // 500 ms
        flashinwork = !flashinwork;
        }
      }  
    }
    osticker = false;
    if (oscounter == 1200) oscounter = 0;   // 5 min 
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Standby oder Mess-Betrieb
bool standby_control() {
  if (sys.stby) {

    drawLoading();                    // Refresh Battery State
    if (!ladenshow) {  
      ladenshow = true;
      IPRINTPLN("Standby");
      //stop_wifi();  // führt warum auch immer bei manchen Nanos zu ständigem Restart
      disableAllHeater();             // Stop Pitmaster
      server.reset();                 // Stop Webserver
      piepserOFF();                   // Stop Pieper
    }

    maintimer(1);                     // Check if Standby
    return 1;
  }
  return 0;
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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Date String to Date Element
// Quelle: https://github.com/oh1ko/ESP82666_OLED_clock/blob/master/ESP8266_OLED_clock.ino
tmElements_t * string_to_tm(tmElements_t *tme, char *str) {
  // Sat, 28 Mar 2015 13:53:38 GMT

  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  char *r, *i, *t;
  r = strtok_r(str, " ", &i);

  r = strtok_r(NULL, " ", &i);
  tme->Day = atoi(r);

  r = strtok_r(NULL, " ", &i);
  for (int i = 0; i < 12; i++) {
    if (!strcmp(months[i], r)) {
      tme->Month = i + 1;
      break;
    }
  }
  
  r = strtok_r(NULL, " ", &i);
  tme->Year = atoi(r) - 1970;

  r = strtok_r(NULL, " ", &i);
  t = strtok_r(r, ":", &i);
  tme->Hour = atoi(t);

  t = strtok_r(NULL, ":", &i);
  tme->Minute = atoi(t);

  t = strtok_r(NULL, ":", &i);
  tme->Second = atoi(t);

  return tme;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Nachkommastellen limitieren
float limit_float(float f, int i) {
  if (i >= 0) {
    if (ch[i].temp!=INACTIVEVALUE) {
      f = f + 0.05;                   // damit er "richtig" rundet, bei 2 nachkommastellen 0.005 usw.
      f = (int)(f*10);               // hier wird der float *10 gerechnet und auf int gecastet, so fallen alle weiteren Nachkommastellen weg
      f = f/10;
    } else f = 999;
  } else {
    f = f + 0.005;
    f = (int)(f*100);
    f = f/100;
  }
  return f;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MAC-Adresse
String getMacAddress()  {
  uint8_t mac[6];
  char macStr[18] = { 0 };
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return  String(macStr);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Cloud Token Generator
String newToken() {
  String stamp = String(now(), HEX);
  int x = 10 - stamp.length();          //pow(16,(10 - timestamp.length()));
  long y = 1;    // long geht bis 16^7
  if (x > 7) {
    stamp += String(random(268435456), HEX);
    x -= 7;
  }
  for (int i=0;i<x;i++) y *= 16;
  stamp += String(random(y), HEX);
  return (String) String(ESP.getChipId(), HEX) + stamp;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GET/POST-Request

#define SAVEDATALINK "/saveData.php"
#define SAVELOGSLINK "/saveLogs.php"
#define SENDTSLINK "/update.json"
#define SENDTHINGSPEAK "Thingspeak"
#define THINGSPEAKSERVER "api.thingspeak.com"
#define NANOSERVER "nano.wlanthermo.de"
#define UPDATESERVER "update.wlanthermo.de"   // früher nano.wlanthermo.de
#define CLOUDSERVER "cloud.wlanthermo.de"
#define MESSAGESERVER "message.wlanthermo.de" 
#define SENDNOTELINK "/message.php"
#define THINGHTTPLINK "/apps/thinghttp/send_request"
#define CHECKUPDATELINK "/checkUpdate.php"

enum {SERIALNUMBER, APITOKEN, TSWRITEKEY, NOTETOKEN, NOTEID, NOTEREPEAT, NOTESERVICE,
      THINGHTTPKEY, DEVICE, HARDWAREVS, SOFTWAREVS, ITEM};  // Parameters
enum {NOPARA, SENDTS, SENDNOTE, THINGHTTP, CHECKUPDATE};                       // Config
enum {GETMETH, POSTMETH};                                                   // Method

String createParameter(int para) {

  String command;
  switch (para) {

    case SERIALNUMBER:
      command += F("serial=");
      command += String(ESP.getChipId(), HEX);
      break;

    case APITOKEN:
      command += F("&api_token=");
      command += iot.CL_token;
      break;

    case TSWRITEKEY:
      command += F("api_key=");
      command += iot.TS_writeKey;
      break;

    case NOTETOKEN:
      command += F("&token=");
      command += pushd.token;
      break;

    case NOTEID:
      command += F("&chatID=");
      command += pushd.id;
      break;

    case NOTEREPEAT:
      command += F("&repeat=");
      command += pushd.repeat;
      break;

    case NOTESERVICE:
      command += F("&service=");
      switch (pushd.service) {
        case 0: command += F("telegram"); break;
        case 1: command += F("pushover"); break;
        case 2: command += F("prowl"); break;
      }
      if (pushd.on == 2) pushd.on = 3;                    // alte Werte wieder herstellen  
      break;

    case THINGHTTPKEY:
      command += F("api_key=");
      command += iot.TS_httpKey;
      break;

    case DEVICE:
      command += F("&device=nano");
      break;

    case HARDWAREVS:
      command += F("&hw_version=v");
      command += String(sys.hwversion);
      break;

    case SOFTWAREVS:
      command += F("&sw_version=");
      command += FIRMWAREVERSION;
      break;

    case ITEM:
      command += F("&product=");
      command += sys.item;
      break;
  }

  return command;
}

String createCommand(bool meth, int para, const char * link, const char * host, int content) {

  String command;
  command += meth ? F("POST ") : F("GET ");
  command += String(link);
  command += (para != NOPARA) ? "?" : "";

  switch (para) {

    case SENDTS:
      command += createParameter(TSWRITEKEY);
      command += collectData();
      break;

    case SENDNOTE:
      command += createParameter(SERIALNUMBER);
      command += createParameter(NOTETOKEN);
      command += createParameter(NOTEID);
      command += createParameter(NOTEREPEAT);
      command += F("&lang=de");
      command += createParameter(NOTESERVICE);
      command += createNote(0);
      break;

    case THINGHTTP:
      command += createParameter(THINGHTTPKEY);
      command += createNote(1);
      break;

    case CHECKUPDATE:
      command += createParameter(SERIALNUMBER);
      command += createParameter(DEVICE);
      command += createParameter(HARDWAREVS);
      command += createParameter(SOFTWAREVS);
      if (sys.item != "") 
        command += createParameter(ITEM);            
      break;

    default:
    break;
      
  }

  command += F(" HTTP/1.1\n");

  if (content > 0) {
    command += F("Content-Type: application/json\n");
    command += F("Content-Length: ");
    command += String(content);
    command += F("\n");
  }

  command += F("User-Agent: WLANThermo nano\n");
  command += F("SN: "); command += String(ESP.getChipId(), HEX); command += F("\n"); 
  command += F("Host: ");
  command += String(host);
  command += F("\n\n");

  return  command;
}

void sendNotification() {
  
  if (wifi.mode == 1) {                   // Wifi available

    if (notification.type > 0) {                      // GENERAL NOTIFICATION       
        
      if (pushd.on > 0) {
        if (sendNote(0)) sendNote(2);           // Notification per Nano-Server
      }
        
    } else if (notification.index > 0) {              // CHANNEL NOTIFICATION

      for (int i=0; i < CHANNELS; i++) {
        if (notification.index & (1<<i)) {            // ALARM AT CHANNEL i
            
          bool sendN = true;
          if (iot.TS_httpKey != "" && iot.TS_on) {
            if (sendNote(0)) {
              notification.ch = i;
              sendNote(1);           // Notification per Thingspeak
            } else sendN = false;
          } else if (pushd.on > 0) {
            if (sendNote(0)) {
              notification.ch = i;
              sendNote(2);           // Notification per Nano-Server
            } else sendN = false;
          }
          if (sendN) {
            notification.index &= ~(1<<i);           // Kanal entfernen, sonst erneuter Aufruf
            return;                                  // nur ein Senden pro Durchlauf
          }
        }
      }    
    }
  }

  if (pushd.on == 3) loadconfig(ePUSH,0);     // nach Testnachricht alte Werte wieder herstellen
}


void serverAnswer(String payload, size_t len) {
 
  if (payload.indexOf("200 OK") > -1) {
    DPRINTP("[HTTP]\tServer Answer: "); 
    int index = payload.indexOf("\r\n\r\n");       // Trennung von Header und Body
    payload = payload.substring(index+7,len);      // Beginn des Body
    index = payload.indexOf("\r");                 // Ende Versionsnummer
    payload = payload.substring(0,index);
    DPRINTLN(payload);
  }
}

void printRequest(uint8_t* datas) {
  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);
}

enum {CONNECTFAIL, SENDTO, DISCONNECT, CLIENTERRROR, CLIENTCONNECT};

void printClient(const char* link, int arg) {

  switch (arg) {

    case CONNECTFAIL:   IPRINTP("f: ");    // Client Connect Fail
      break;

    case SENDTO:        IPRINTP("s:");      // Client Send to
      break;

    case DISCONNECT:    IPRINTP("d:");     //Disconnect Client
      break;

    case CLIENTERRROR:  IPRINTP("f:");     // Client Connect Error:
      break; 

    case CLIENTCONNECT: IPRINTP("c: ");    // Client Connect
      break; 
  }
  DPRINTLN(link);
}


uint16_t getDC(uint16_t impuls) {
  // impuls = value * 10  // 1.Nachkommastelle
  float val = ((float)(impuls - SERVOPULSMIN*10)/(SERVOPULSMAX - SERVOPULSMIN))*100;
  return (val < 500)?ceil(val):floor(val);   // nach oben : nach unten
}



