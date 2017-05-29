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

// EXECPTION LIST
// https://links2004.github.io/Arduino/dc/deb/md_esp8266_doc_exception_causes.html

// Entwicklereinstellungen
#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES
#define THINGSPEAK                          // ENABLE THINGSPEAK
//#define KTYPE                             // ENABLE TYP K (Test only)

#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DPRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define DPRINT(...)     //blank line
  #define DPRINTLN(...)   //blank line
  #define DPRINTF(...)    //blank line
#endif


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// TIMER VARIABLES

unsigned long lastUpdateBatteryMode;
unsigned long lastUpdateSensor;
unsigned long lastUpdatePiepser;
unsigned long lastUpdateCommunication;
unsigned long lastUpdateDatalog;
unsigned long lastFlashInWork;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// INCLUDE SUBROUTINES

#include "c_init.h"
#include "c_button.h"
#include "c_median.h"
#include "c_sensor.h"
#include "c_pitmaster.h"
#include "c_temp.h"
#include "c_fs.h"
#include "c_com.h"
#include "c_ee.h"
#include "c_icons.h"
#include "c_wifi.h"
#include "c_frames.h"
#include "c_bot.h"
#include "c_ota.h"
#include "c_server.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SETUP
void setup() {  

  // Initialize Serial 
  set_serial(); //Serial.setDebugOutput(true);
  
  // Initialize OLED
  set_OLED();

  // Current Battery Voltage
  get_Vbat();
  
  if (!stby) {

    // Open Config-File
    check_sector();
    setEE(); start_fs();
    
    // Initialize Wifi
    set_wifi();

    // Update Time
    if (!isAP)  setTime(getNtpTime()); //setSyncProvider(getNtpTime);

    DPRINT("[INFO]\t");
    DPRINTLN(digitalClockDisplay(now()));

    // Scan Network
    WiFi.scanNetworks(true);
    scantime = millis();
    //scantime = String(now());

    // Initialize Server
    server_setup();
    
    // Initialize OTA
    #ifdef OTA  
      set_ota();
      ArduinoOTA.begin();
    #endif
    
    // Initialize Sensors
    set_sensor();
    set_Channels();
    set_piepser();

    // Initialize Buttons
    set_button();
        
    // Current Wifi Signal Strength
    get_rssi();
    cal_soc();
    
    // Initialize Pitmaster
    set_pitmaster();
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LOOP
void loop() {

  // Standby oder Mess-Betrieb
  if (standby_control()) return;

  // WiFi Monitoring
  wifimonitoring();

  // Detect Serial
  static char serialbuffer[150];
  if (readline(Serial.read(), serialbuffer, 150) > 0) {
    read_serial(serialbuffer);
  }
  
  // Detect OTA
  #ifdef OTA
    ArduinoOTA.handle();
  #endif
  
  // Detect Button Event
  if (button_input()) {
    button_event();
  }
  
  // Update Display
  int remainingTimeBudget;
  if (!displayblocked) {
    remainingTimeBudget = ui.update();
  } else remainingTimeBudget = 1;


  // Timer Actions
  if (remainingTimeBudget > 0) {
    // Don't do stuff if you are below your time budget.

    // Temperture
    if (millis() - lastUpdateSensor > INTERVALSENSOR) {
      get_Temperature();
      get_Vbat();
      lastUpdateSensor = millis();
    }

    // Alarm
    if (millis() - lastUpdatePiepser > INTERVALSENSOR/4) {
      controlAlarm(pulsalarm);
      pulsalarm = !pulsalarm;
      lastUpdatePiepser = millis();
    }
    
    // Pitmaster Control
    pitmaster_control();

    // Communication
    if (millis() - lastUpdateCommunication > INTERVALCOMMUNICATION) {

      get_rssi(); // müsste noch an einen anderen Ort wo es unabhängig von INTERVALCOM.. ist
      cal_soc();
      
      // falls wach und nicht AP
      if (!isAP) {

        #ifdef THINGSPEAK
          if (THINGSPEAK_KEY != "") sendData();
        #endif
          
        #ifdef TELEGRAM
          UserData userData;
          getUpdates(id, &userData);
        #endif
      }
      
      lastUpdateCommunication = millis();
    }

    // Datalog
    if (millis() - lastUpdateDatalog > 2000) {

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
      mylog[logc].timestamp = now();     // 64 bit // 8 byte

      log_count++;
      // 2*8 + 1 + 8 = 25
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

    // Flash
    if (inWork) {
      if (millis() - lastFlashInWork > FLASHINWORK) {
        flashinwork = !flashinwork;
        lastFlashInWork = millis();
      }
    }
    
    //delay(remainingTimeBudget);
    delay(1); // sonst geht das Wifi Modul nicht in Standby
    //yield();  // reicht nicht
  }
  
}


