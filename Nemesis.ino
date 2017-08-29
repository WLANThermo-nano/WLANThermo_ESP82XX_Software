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
#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES

#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DPRINTP(...)   Serial.print(F(__VA_ARGS__))
  #define DPRINTPLN(...) Serial.println(F(__VA_ARGS__))
  #define DPRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define DPRINT(...)     //blank line
  #define DPRINTLN(...)   //blank line 
  #define DPRINTP(...)    //blank line
  #define DPRINTPLN(...)  //blank line
  #define DPRINTF(...)    //blank line
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
#include "c_fs.h"
#include "c_com.h"
#include "c_ee.h"
#include "c_icons.h"
#include "c_wifi.h"
#include "c_frames.h"
#include "c_bot.h"
#include "c_pmqtt.h"
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

    // Initalize Aktor
    set_piepser();

    // GodMode aktiv
    if (sys.god) {
      piepserON(); delay(500); piepserOFF();
    }

    // Initalize P_MQTT
    set_pmqtt();
    
    // Initialize Wifi
    set_wifi();

    // Update Time
    //set_time();
    
    // Scan Network
    WiFi.scanNetworks(true);
    scantime = millis();
    //scantime = String(mynow());

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
    get_rssi();
    cal_soc();
    
    // Initialize Pitmaster
    set_pitmaster(0); 

    // Check HTTP Update
    check_http_update();
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
  static char serialbuffer[300];
  if (readline(Serial.read(), serialbuffer, 300) > 0) {
    read_serial(serialbuffer);
  }
  
  // Detect OTA
  #ifdef OTA
    ArduinoOTA.handle();
  #endif

  // HTTP Update
  if (sys.update > 0) do_http_update();
  else if (sys.update == -1) check_http_update();
  
  // Detect Button Event
  if (button_input()) button_event();
  
  // Update Display
  int remainingTimeBudget;
  if (!displayblocked)  remainingTimeBudget = ui.update();
  else remainingTimeBudget = 1;

  // Timer Actions
  if (remainingTimeBudget > 0) {
    // Don't do stuff if you are below your time budget.

    timer_sensor();           // Temperture
    timer_alarm();            // Alarm
    pitmaster_control();      // Pitmaster
    timer_iot();              // Charts
    timer_datalog();          // Datalog
    flash_control();          // Flash
    
    delay(5);   // sonst geht das Wifi Modul nicht in Standby, yield() reicht nicht!
  }
  
}


