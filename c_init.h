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
#define FIRMWAREVERSION "v0.8.0"
#define APIVERSION      "v1"

// CHANNELS
#define CHANNELS 8                     // UPDATE AUF HARDWARE 4.05
#define INACTIVEVALUE  999             // NO NTC CONNECTED
#define SENSORTYPEN    11               // NUMBER OF SENSORS
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
#define EEPROM_SIZE 2176            // EEPROM SIZE
#define EEWIFIBEGIN         0
#define EEWIFI              300
#define EESYSTEMBEGIN       EEWIFIBEGIN+EEWIFI
#define EESYSTEM            250
#define EECHANNELBEGIN      EESYSTEMBEGIN+EESYSTEM
#define EECHANNEL           500
#define EETHINGBEGIN        EECHANNELBEGIN+EECHANNEL
#define EETHING             420
#define EEPITMASTERBEGIN    EETHINGBEGIN+EETHING
#define EEPITMASTER         700

// PITMASTER
#define PITMASTER1 15               // PITMASTER PIN
#define PITMASTER2 14               // CLK // ab Platine V7.2
#define PITSUPPLY  12               // MISO // ab Platine V9.3
#define PITMIN 0                    // LOWER LIMIT SET
#define PITMAX 100                  // UPPER LIMIT SET
#define PITMASTERSIZE 5             // PITMASTER SETTINGS LIMIT
#define PITMASTERSETMIN 50
#define PITMASTERSETMAX 200

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

// SENSORTYP
String  ttypname[SENSORTYPEN] = {"Maverick","Fantast-Neu","Fantast","iGrill2","ET-73",
                                 "Perfektion","5K3A1B","MOUSER47K","100K6A1B","Weber_6743",
                                 "Santos"};
// TEMPERATURE UNIT
String  temp_unit = "C";

// CHANNEL COLORS
String colors[8] = {"#0C4C88","#22B14C","#EF562D","#FFC100","#A349A4","#804000","#5587A2","#5C7148"};

// PITMASTER
struct Pitmaster {
   byte pid;           // PITMASTER PID-Setting
   float set;            // SET-TEMPERATUR
   bool  active;           // IS PITMASTER ACTIVE
   byte  channel;         // PITMASTER CHANNEL
   float value;           // PITMASTER VALUE IN %
   int manual;            // MANUEL PITMASTER VALUE IN %
   bool event;
   int16_t msec;          // PITMASTER VALUE IN MILLISEC
   unsigned long last;
   int pause;             // PITMASTER PAUSE
   bool resume;           // Continue after restart 
   long timer0;           
};
Pitmaster pitmaster;
int pidsize;

// PID PROFIL
struct PID {
  String name;
  byte id;
  byte aktor;                     // 0: SSR, 1:FAN, 2:Servo
  //byte port;                  // IO wird über typ bestimmt
  float Kp;                     // P-Konstante oberhalb pswitch
  float Ki;                     // I-Konstante oberhalb pswitch
  float Kd;                     // D-Konstante oberhalb pswitch
  float Kp_a;                   // P-Konstante unterhalb pswitch
  float Ki_a;                   // I-Konstante unterhalb pswitch
  float Kd_a;                   // D-Konstante unterhalb pswitch
  int Ki_min;                   // Minimalwert I-Anteil
  int Ki_max;                   // Maximalwert I-Anteil
  float pswitch;                // Umschaltungsgrenze
  bool reversal;                // VALUE umkehren
  int DCmin;                    // Duty Cycle Min
  int DCmax;                    // Duty Cycle Max
  int SVmin;                    // SERVO IMPULS MIN
  int SVmax;                    // SERVO IMPULS MAX
  float esum;                   // Startbedingung I-Anteil
  float elast;                  // Startbedingung D-Anteil
  
};
PID pid[PITMASTERSIZE];

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

// DUTYCYCLE
struct DutyCycle {
  long timer;
  int value;
  bool dc;
  byte aktor;
  bool on;
  int saved;
};

DutyCycle dutycycle;

// DATALOGGER
struct datalogger {
 uint16_t tem[8];     //8
 long timestamp;
 uint8_t pitmaster;
 uint16_t soll;
 uint8_t battery;
 bool modification;
};

#define MAXLOGCOUNT 10 //155             // SPI_FLASH_SEC_SIZE/ sizeof(datalogger)
datalogger mylog[MAXLOGCOUNT];
datalogger archivlog[MAXLOGCOUNT];
unsigned long log_count = 0;
int log_checksum = 0;
uint32_t log_sector;                // erster Sector von APP2
uint32_t freeSpaceStart;            // First Sector of OTA
uint32_t freeSpaceEnd;              // Last Sector+1 of OTA

// NOTIFICATION
struct Notification {
  byte ch;                          // CHANNEL
  bool limit;                       // LIMIT: 0 = LOW TEMPERATURE, 1 = HIGH TEMPERATURE
};

Notification notification;

// SYSTEM
struct System {
   byte hwversion;           // HARDWARE VERSION
   bool fastmode;              // FAST DISPLAY MODE
   String apname;             // AP NAME
   String host;                     // HOST NAME
   String language;           // SYSTEM LANGUAGE
   bool hwalarm;              // HARDWARE ALARM 
   byte updatecount;           // 
   int update;             // FIRMWARE UPDATE -1 = check, 0 = no, 1 = spiffs, 2 = firmware
   String getupdate;
   bool autoupdate;
   bool god;
   bool pitsupply;        
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
   int TG_on;                   // TELEGRAM NOTIFICATION SERVICE
   String TG_token;             // TELEGRAM API TOKEN
   String TG_id;                // TELEGRAM CHAT ID 
   bool CL_on;                  // NANO CLOUD ON / OFF
   String CL_token;             // NANO CLOUD TOKEN
   int CL_int;                  // NANO CLOUD INTERVALL
};

IoT iot;

// CLOUD CHART/LOG
struct Chart {
   bool on = false;                  // NANO CHART ON / OFF
//   String token;             // NANO CHART TOKEN
//   int interval;                  // NANO CHART INTERVALL
};

Chart chart;

// OLED
int current_ch = 0;               // CURRENTLY DISPLAYED CHANNEL     
bool LADENSHOW = false;           // LOADING INFORMATION?
bool displayblocked = false;                     // No OLED Update
enum {NO, CONFIGRESET, CHANGEUNIT, OTAUPDATE, HARDWAREALARM, IPADRESSE, AUTOTUNE};

// OLED QUESTION
struct MyQuestion {
   int typ;    
   int con;            
};

MyQuestion question;

// FILESYSTEM
enum {eCHANNEL, eWIFI, eTHING, ePIT, eSYSTEM, ePRESET};

// WIFI
ESP8266WiFiMulti wifiMulti;               // MULTIWIFI instance
WiFiUDP udp;                              // UDP instance
byte isAP = 2;                    // WIFI MODE  (0 = STA, 1 = AP, 2 = NO, 3 = Turn off)
unsigned long isAPcount;
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
byte subframepos[4] = {1, 6, 11, 19};    // immer ein Back dazwischen
int current_frame = 0;  
bool flashinwork = true;
float tempor;                       // Zwischenspeichervariable

// WEBSERVER
AsyncWebServer server(80);        // https://github.com/me-no-dev/ESPAsyncWebServer
const char* www_username = "admin";
const char* www_password = "admin";

// TIMER
unsigned long lastUpdateBatteryMode;
unsigned long lastUpdateSensor;
unsigned long lastUpdatePiepser;
unsigned long lastUpdateDatalog;
unsigned long lastFlashInWork;
unsigned long lastUpdateRSSI;
unsigned long lastUpdateThingspeak;
unsigned long lastUpdateCloud;
unsigned long lastUpdateLog;
unsigned long lastUpdateMQTT;

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
void get_Vbat();                                   // Reading Battery Voltage
void cal_soc();

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
WiFiEventHandler wifiConnectHandler;

//MQTT
AsyncMqttClient pmqttClient;
void sendpmqtt();

// EEPROM
void setEE();
void writeEE(const char* json, int len, int startP);
void readEE(char *buffer, int len, int startP);
void clearEE(int startP, int endP);

// PITMASTER
void startautotunePID(int maxCyc, bool store, int over, long tlimit);
void pitmaster_control();
void disableAllHeater();
void set_pitmaster(bool init);
void set_pid();
void stopautotune();

// BOT
void set_iot(bool init);
String collectData();
String createNote(bool ts);
bool sendNote(int check);
void sendSettings();
void sendDataTS();

void sendServerLog();
String serverLog();
void sendDataCloud();

String cloudData();

// ++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Serial
void set_serial() {
  Serial.begin(115200);
  DPRINTLN();
  DPRINTLN();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize System-Settings, if not loaded from EE
void set_system() {
  
  String host = HOSTNAME;
  host += String(ESP.getChipId(), HEX);
  
  sys.host = host;
  sys.hwalarm = false; 
  sys.apname = APNAME;
  sys.language = "de";
  sys.fastmode = false;
  sys.hwversion = 1;
  if (sys.update == 0) sys.getupdate = "false";   // Änderungen am EE während Update
  sys.autoupdate = 1;
  sys.god = false;
  battery.max = BATTMAX;
  battery.min = BATTMIN;
  sys.pitsupply = false;
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
// IoT Timer
void timer_iot() {

  // THINGSPEAK
  if (millis() - lastUpdateThingspeak > (iot.TS_int * 1000)) {

    if (!isAP && sys.update == 0 && iot.TS_on) {
      if (iot.TS_writeKey != "" && iot.TS_chID != "") sendDataTS();
    }
    lastUpdateThingspeak = millis();
  }

  // PRIVATE MQTT
  if (millis() - lastUpdateMQTT > (iot.P_MQTT_int * 1000)) {

    if (!isAP && sys.update == 0 && iot.P_MQTT_on) sendpmqtt();
    lastUpdateMQTT = millis();
  }

  // NANO CLOUD
  if (millis() - lastUpdateCloud > (iot.CL_int * 1000)) {

    if (!isAP && sys.update == 0 && iot.CL_on) {
        sendDataCloud();
    }
    lastUpdateCloud = millis();
  }

  // NANO LOGS
  if (millis() - lastUpdateLog > INTERVALCOMMUNICATION) {

    if (!isAP && sys.update == 0 && chart.on) {
        sendServerLog();
    }
    lastUpdateLog = millis();
  }
  
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// DataLog Timer
void timer_datalog() {  
  
  if (millis() - lastUpdateDatalog > 3000) {

    int logc;
    int checksum = 0;
    
    if (log_count < MAXLOGCOUNT) logc = log_count;
    else {
      logc = MAXLOGCOUNT-1;  // Array verschieben
      memcpy(&mylog[0], &mylog[1], (MAXLOGCOUNT-1)*sizeof(*mylog));
    }

    for (int i=0; i < CHANNELS; i++)  {
      if (ch[i].temp != INACTIVEVALUE) {
        mylog[logc].tem[i] = (uint16_t) (ch[i].temp * 10);    // 8 * 16 bit  // 8 * 2 byte
        checksum += mylog[logc].tem[i];
      } else
        mylog[logc].tem[i] = NULL;
    }
    mylog[logc].pitmaster = (uint8_t) pitmaster.value;            // 8  bit // 1 byte
    if (pitmaster.active) {
      mylog[logc].soll = (uint16_t) (pitmaster.set * 10);           // 16 bit // 2 byte
      checksum += mylog[logc].soll;
    } else  mylog[logc].soll = NULL;
    mylog[logc].timestamp = now();                                // 64 bit // 8 byte
    mylog[logc].battery = (uint8_t) battery.percentage;           // 8  bit // 1 byte

    checksum += mylog[logc].pitmaster;
    checksum += mylog[logc].battery;

    if (checksum == log_checksum) mylog[logc].modification = false;
    else mylog[logc].modification = true;
    log_checksum = checksum;
    
    log_count++;
    // 2*8 + 1 + 2 + 8 + 1 = 28

    /*
    if (log_count%MAXLOGCOUNT == 0 && log_count != 0) {
        
      if (log_sector > freeSpaceEnd/SPI_FLASH_SEC_SIZE) 
        log_sector = freeSpaceStart/SPI_FLASH_SEC_SIZE;
        
      write_flash(log_sector);
      log_sector++;
      setconfig(eSYSTEM,{});  
    }
    */
    
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

/*
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Update Time
void set_time() {
  if (!isAP) {
    while (now() < 30) {        // maximal 30 sec suchen
      time_t present = getNtpTime();
      if (present) setTime(present); 
    }
  }
  
  //setSyncProvider(getNtpTime);
  DPRINTP("[INFO]\t");
  DPRINTLN(digitalClockDisplay(mynow()));
}
*/


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
      //pitmaster.active = false;
      disableAllHeater();
      server.reset();   // Webserver leeren
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
#define UPDATESERVER "update.wlanthermo.de"
#define CLOUDSERVER "cloud.wlanthermo.de"
#define MESSAGESERVER "message.wlanthermo.de" 
#define SENDNOTELINK "/message.php"
#define THINGHTTPLINK "/apps/thinghttp/send_request"
#define CHECKUPDATELINK "/checkUpdate.php"

enum {SERIALNUMBER, APITOKEN, TSWRITEKEY, NOTETOKEN, NOTEID, NOTESERVICE,
      THINGHTTPKEY, DEVICE, HARDWAREVS, SOFTWAREVS};  // Parameters
enum {NOPARA, SAVEDATA, SENDTS, SENDNOTE, THINGHTTP, CHECKUPDATE};                       // Config
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
      command += iot.TG_token;
      break;

    case NOTEID:
      command += F("&chatID=");
      command += iot.TG_id;
      break;

    case NOTESERVICE:
      command += F("&service=");
      command += "telegram";  //iot.TG_on;
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
  }

  return command;
}

String createCommand(bool meth, int para, const char * link, const char * host, int content) {

  String command;
  command += meth ? F("POST ") : F("GET ");
  command += String(link);
  command += (para != NOPARA) ? "?" : "";

  switch (para) {
    
    case SAVEDATA:
      command += createParameter(SERIALNUMBER);
      command += createParameter(APITOKEN);
      break;

    case SENDTS:
      command += createParameter(TSWRITEKEY);
      command += collectData();
      break;

    case SENDNOTE:
      command += createParameter(SERIALNUMBER);
      command += createParameter(NOTETOKEN);
      command += createParameter(NOTEID);
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

  command += F("User-Agent: ESP8266\n");
  command += F("Host: ");
  command += String(host);
  command += F("\n\n");

  return  command;
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

    case CONNECTFAIL:   DPRINTP("[INFO]\tClient Connect Fail: ");
      break;

    case SENDTO:        DPRINTP("[INFO]\tClient Send to:");
      break;

    case DISCONNECT:    DPRINTP("[INFO]\tDisconnect Client: ");
      break;

    case CLIENTERRROR:  DPRINTP("[INFO]\tClient Connect Error: ");
      break; 

    case CLIENTCONNECT: DPRINTP("[INFO]\tClient Connect: ");
      break; 
  }
  DPRINTLN(link);
}


