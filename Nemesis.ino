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
// WATCHDOG
// https://techtutorialsx.com/2017/01/21/esp8266-watchdog-functions/

// Entwicklereinstellungen
//#define ASYNC_TCP_SSL_ENABLED 1
//#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES
//#define MPR
 
#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DPRINTP(...)   Serial.print(F(__VA_ARGS__))
  #define DPRINTPLN(...) Serial.println(F(__VA_ARGS__))
  #define DPRINTF(...)   Serial.printf(__VA_ARGS__)
  #define IPRINT(...)    Serial.print("[INFO]\t");Serial.print(__VA_ARGS__)
  #define IPRINTLN(...)  Serial.print("[INFO]\t");Serial.println(__VA_ARGS__)
  #define IPRINTP(...)   Serial.print("[INFO]\t");Serial.print(F(__VA_ARGS__))
  #define IPRINTPLN(...) Serial.print("[INFO]\t");Serial.println(F(__VA_ARGS__))
  #define IPRINTF(...)   Serial.print("[INFO]\t");Serial.printf(__VA_ARGS__)
  
#else
  #define DPRINT(...)     //blank line
  #define DPRINTLN(...)   //blank line 
  #define DPRINTP(...)    //blank line
  #define DPRINTPLN(...)  //blank line
  #define DPRINTF(...)    //blank line
  #define IPRINT(...)     //blank line
  #define IPRINTLN(...)   //blank line
  #define IPRINTP(...)    //blank line
  #define IPRINTPLN(...)  //blank line
  #define IPRINTF(...)    //blank line
#endif


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// INCLUDE SUBROUTINES

#include "c_init.h"
#include "c_webhandler.h"
#include "c_button.h"
#include "c_median.h"
#include "c_sensor.h"
#include "c_pitmaster.h"
#include "c_temp.h"
#include "c_ee.h"
#include "c_fs.h"
#include "c_com.h"
#include "c_icons.h"
#include "c_wifi.h"
#include "c_frames.h"
#include "c_bot.h"
#include "c_pmqtt.h"
#include "c_ota.h"
#include "c_server.h"
#include "c_api.h"
//#include "c_ws.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SETUP
void setup() {  

  //delay(1000);

  // Initialize Serial 
  set_serial(); Serial.setDebugOutput(true);
  set_ostimer();
  
  // Initialize OLED
  set_OLED();

  // Open Config-File
  check_sector();
  setEE(); start_fs();

  // Current Battery Voltage
  get_Vbat(); get_rssi();

  if (!sys.stby) {
    
    // Initalize Aktor
    set_piepser();

    // GodMode aktiv
    if (sys.god & (1<<0)) {
      piepserON(); delay(500); piepserOFF();
    }
  
    // Initalize P_MQTT
    set_pmqtt();
    
    // Initialize Wifi
    set_wifi();

    // Initialize Server
    server_setup();
    
    // Initialize OTA
    #ifdef OTA  
      set_ota();
      ArduinoOTA.begin();
    #endif
    
    // Initialize Sensors
    set_sensor();
    set_channels(0);

    // Initialize Buttons
    set_button();
        
    // Current Wifi Signal Strength
    get_Vbat();
    get_rssi();
    cal_soc();
    
    // Initialize Pitmaster
    set_pitmaster(0); 

    // Check Reset Info
    if (checkResetInfo()) {
      //if (SPIFFS.remove(LOG_FILE)) Serial.println("Neues Log angelegt");
    }

  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LOOP
void loop() {

  // Detect Serial Input
  static char serialbuffer[300];
  if (readline(Serial.read(), serialbuffer, 300) > 0) {
    read_serial(serialbuffer);
  }
  
  // Standby oder Mess-Betrieb
  if (standby_control()) return;

  // Close Start Screen
  if (question.typ == SYSTEMSTART && millis() > 3000) {
    displayblocked = false;   // Close Start Screen (if not already done)
    question.typ = NO;
  }

  // Manual Restart
  if (sys.restartnow) {
    if (wifi.mode == 5) WiFi.disconnect();
    delay(100);
    yield();
    ESP.restart();
  }

  // WiFi Monitoring
  wifimonitoring();
  
  
  // Detect OTA
  #ifdef OTA
    ArduinoOTA.handle();
  #endif

  // HTTP Update
  check_api();
  if (update.state > 0) do_http_update();
  //else if (update.state == -1) check_http_update();
  
  // Detect Button Event
  if (button_input()) button_event();
  
  // Update Display
  int remainingTimeBudget;
  if (!displayblocked)  remainingTimeBudget = ui.update();
  else remainingTimeBudget = 1;

  // Timer Actions
  if (remainingTimeBudget > 0) {
    // Don't do stuff if you are below your time budget.

    maintimer();
    pitmaster_control(0);      // Pitmaster 1
    pitmaster_control(1);      // Pitmaster 2
    sendNotification();       // Notification
    
    //savelog();
    //ampere_control();
    
    checkMqtt();

    updateServo();
    if (servointerrupt) {   // nur innerhalb eines Servo-Takts
      delay(10);   // sonst geht das Wifi Modul nicht in Standby, yield() reicht nicht!
    }
  }
  
}


