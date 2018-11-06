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

// number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))


// ++++++++++++++++++++++++++++++++++++++++++++++++++
// SETTINGS
// Nano V2: MISO > Supply Switch; CLK > PIT2
// ++++++++++++++++++++++++++++++++++++++++++++++++++


// HARDWARE
#define FIRMWAREVERSION  "v1.0.0-beta"
#define GUIAPIVERSION    "1"
#define SERVERAPIVERSION "1"

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
#define APNAME      "NANO-AP"
#define APPASSWORD  "12345678"
#define HOSTNAME    "NANO-"

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

// API
#define APISERVER "api.wlanthermo.de"
#define CHECKAPI "/"

// THINGSPEAK
#define THINGSPEAKSERVER "api.thingspeak.com"
#define SENDTSLINK "/update.json"
#define THINGHTTPLINK "/apps/thinghttp/send_request"


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// GLOBAL VARIABLES
// ++++++++++++++++++++++++++++++++++++++++++++++++++++

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
   bool repeatalarm;
   int repeat;
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
  byte autotune;
};
PID pid[PIDSIZE];

// AUTOTUNE
struct AutoTune {
   uint32_t set;                   // BETRIEBS-TEMPERATUR
   uint32_t time[3];            // TIME VECTOR
   float temp[3];               // TEMPERATURE VECTOR
   float value;                 // CURRENT AUTOTUNE VALUE
   byte run;                    // WAIT FOR AUTOTUNE START: 0:off, 1:init, 2:run
   byte stop;                   // STOP AUTOTUNE: 1: normal, 2: overtemp, 3: timeout
   int overtemp = 30;            // OVERTEMPERATURE LIMIT
   uint32_t timelimit = 120L*60000L;               // TIMELIMIT
   float Kp;
   float Ki;
   float Kd;
   //bool keepup;             // PITMASTER FORTSETZEN NACH ENDE
   float vmax;
   uint8_t max;               // MAXIMAL AUTOTUNE PITMASTER VALUE
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

// UPDATE
struct myUpdate {
  String firmwareUrl;             // UPDATE FIRMWARE LINK
  String spiffsUrl;               // UPDATE SPIFFS LINK
  byte count;                     // UPDATE SPIFFS REPEAT
  int state;                      // UPDATE STATE: -1 = check, 0 = no, 1 = start spiffs, 2 = check after restart, 3 = firmware, 4 = finish
  String get;                     // UPDATE MY NEW VERSION (über Eingabe)
  String version;                 // UPDATE SERVER NEW VERSION
  bool autoupdate;                // CHECK UPDATE INFORMATION
  bool prerelease;                // ?
};

myUpdate update;

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
enum {eCHANNEL, eWIFI, eTHING, ePIT, eSYSTEM, ePUSH, eSERVER, ePRESET};


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

// URL
struct ServerData {
   String host;           // nur die Adresse ohne Anhang
   String page;           // alles was nach de, com etc. kommt  
   String typ; 
};

ServerData serverurl[5];     // 0:api, 1: note, 2:cloud
enum {APILINK, NOTELINK, CLOUDLINK, TSLINK, HTTPLINK};

enum {NOPARA, TESTPARA, SENDTS, THINGHTTP};                       // Config GET/POST Request


rst_info *myResetInfo;

#ifdef MEMORYCLOUD

  #define CLOUDLOGMAX 2   // Cloud 1/3

  // DATALOGGER
  struct Datalogger {
    uint16_t tem[CHANNELS];
    //String color[CHANNELS];
    //long timestamp;
    uint8_t value;
    uint16_t set;
    byte status;
    uint8_t soc;
  };

  Datalogger cloudlog[CLOUDLOGMAX];
  int cloudcount;

  void saveLog();
  
#endif

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
void check_api();
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
String connectionStatus (int which);

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
void startautotunePID(byte id);
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
void sendNotification();
String newToken();

#ifdef THINGSPEAK
String collectData();
String createNote();
#endif

// API
int apiindex;
int urlindex;
int parindex;
void urlObj(JsonObject  &jObj);
void dataObj(JsonObject &jObj, bool cloud);
bool sendAPI(int check);
String apiData(int typ);
enum {NOAPI, APIUPDATE, APICLOUD, APIDATA, APISETTINGS, APINOTE, APIALEXA};

//void setWebSocket();

// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// BASIC FUNCTIONS

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
  if (update.state == 0) {
    update.get = "false";   // Änderungen am EE während Update
    update.version = "false";
  }
  update.autoupdate = 1;
  update.firmwareUrl = "";          // wird nur von der API befüllt wenn Update da ist
  update.spiffsUrl = "";
  sys.god = false;
  sys.typk = false;
  battery.max = BATTMAX;
  battery.min = BATTMIN;
  battery.setreference = 0;
  sys.pitsupply = false;           // nur mit Mod
  sys.damper = false;
  sys.restartnow = false;

  update.state = -1;  // Kontakt zur API herstellen
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

      // PRIVATE MQTT
      if (!(oscounter % (iot.P_MQTT_int*4))) {   // variable
        if (wifi.mode == 1 && update.state == 0 && iot.P_MQTT_on) sendpmqtt();
      } 

      // NOTIFICATION (kein Timer notwenig)   (kann noch verbessert werden, siehe sendNotification())
      if (!(oscounter % 1)) { 
        if (wifi.mode == 1 && update.state == 0) sendNotification();
      }

      // NANO CLOUD (nach Notification)
      if (!(oscounter % (iot.CL_int*4)) || lastUpdateCloud) {   // variable
        if (wifi.mode == 1 && update.state == 0 && iot.CL_on) {
          if (sendAPI(0)) {
            apiindex = APICLOUD;
            urlindex = CLOUDLINK;
            parindex = NOPARA;
            sendAPI(2);
          } else {
            #ifdef MEMORYCLOUD  
              cloudcount = 0;           // ansonsten von API zurückgesetzt
            #endif 
          }
        }
        lastUpdateCloud = false;
      }

      #ifdef MEMORYCLOUD        // Zurücksetzen einbauen (CL_int, Temp_einheit ...)
      if (!(oscounter % ((iot.CL_int/3)*4))) {    // 
        if (iot.CL_on && cloudcount < CLOUDLOGMAX) saveLog();
      }
      #endif

      #ifdef THINGSPEAK

      // THINGSPEAK
      if (!(oscounter % (iot.TS_int*4))) {   // variable
        if (wifi.mode == 1 && update.state == 0 && iot.TS_on) {
          if (iot.TS_writeKey != "" && iot.TS_chID != "") {
            if (sendAPI(0)) {
              apiindex = NOAPI;
              urlindex = TSLINK;
              parindex = SENDTS;
              sendAPI(2);
            }
          }
        }
      } 

      #endif

      // ALARM REPEAT
      if (!(oscounter % 60)) {     // 15 s
       for (int i=0; i < CHANNELS; i++) {
        if (ch[i].isalarm)  {
          if (ch[i].repeat > 1) {
            ch[i].repeat -= 1;
            ch[i].repeatalarm = true;
          }
        } else ch[i].repeat = pushd.repeat;
       }
      }

      // OLED FLASH TIMER
      if (inWork) {
        if (!(oscounter % FLASHINWORK)) {     // 500 ms
        flashinwork = !flashinwork;
        }
      }  
    }
    osticker = false;
    //Serial.println(oscounter);
    if (oscounter == 2400) oscounter = 0;   // 10 min (muss durch 5, 2, 1, 0,5, 0,25 ganzzahlig teilbar sein)
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
// Server URLs

void setserverurl() {

  serverurl[0].host = APISERVER;
  serverurl[0].page = CHECKAPI;
  serverurl[0].typ  = "api";

  serverurl[1].host = APISERVER;
  serverurl[1].page = CHECKAPI;
  serverurl[1].typ  = "note";

  serverurl[2].host = APISERVER;
  serverurl[2].page = CHECKAPI;
  serverurl[2].typ  = "cloud";

  serverurl[3].host = THINGSPEAKSERVER;
  serverurl[3].page = SENDTSLINK;
  serverurl[3].typ  = "thingspeak";

  serverurl[4].host = THINGSPEAKSERVER;
  serverurl[4].page = THINGHTTPLINK;
  serverurl[4].typ  = "thinghttp";
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GET/POST-Request

enum {SERIALNUMBER, DEVICE, HARDWAREVS, SOFTWAREVS, UPDATEVERSION, TSWRITEKEY, THINGHTTPKEY};  // Parameters

// GET/POST Parameter Generator
String createParameter(int para) {

  String command;
  switch (para) {

    case SERIALNUMBER:
      command += F("serial=");
      command += String(ESP.getChipId(), HEX);
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

    case UPDATEVERSION:
      command += F("&version=");
      command += update.get;
      break;
      
    #ifdef THINGSPEAK
    
    case THINGHTTPKEY:
      command += F("api_key=");
      command += iot.TS_httpKey;
      break;

    case TSWRITEKEY:
      command += F("api_key=");
      command += iot.TS_writeKey;
      break;

    #endif
  }

  return command;
}


//enum {NOPARA, TESTPARA, SENDTS, THINGHTTP};                       // Config
enum {GETMETH, POSTMETH};                                                   // Method

// GET/POST Generator
String createCommand(bool meth, int para, const char * link, const char * host, int content) {

  String command;
  command += meth ? F("POST ") : F("GET ");
  command += String(link);
  command += (para != NOPARA) ? "?" : "";

  switch (para) {

    case TESTPARA:
    break;

    #ifdef THINGSPEAK

    case SENDTS:
      command += createParameter(TSWRITEKEY);
      command += collectData();
      break;

    case THINGHTTP:
      command += createParameter(THINGHTTPKEY);
      command += createNote();
      break;

    #endif

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


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate Duty Cycle Milliseconds

uint16_t getDC(uint16_t impuls) {
  // impuls = value * 10  // 1.Nachkommastelle
  float val = ((float)(impuls - SERVOPULSMIN*10)/(SERVOPULSMAX - SERVOPULSMIN))*100;
  return (val < 500)?ceil(val):floor(val);   // nach oben : nach unten
}



