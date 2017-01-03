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


// CHOOSE CONFIGURATION (user input)

// bitte auskommentieren falls nicht benutzt
#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES

// bitte nicht zutreffendes auskommentieren
//#define VARIANT_A                           // 3xNTC// CHOOSE HARDWARE
#define VARIANT_B                           // 6xNTC, 1xSYSTEM
//#define VARIANT_C                           // 4xNTC, 1xKYTPE, 1xSYSTEM

// falls erstes Flashen "xxx" ersetzen
#define WIFISSID "xxx"              // SET WIFI SSID (falls noch kein wifi.json angelegt ist)  
#define PASSWORD "xxx"              // SET WIFI PASSWORD (falls noch kein wifi.json angelegt ist)

// bitte auskommentieren falls nicht benutzt
//#define TELEGRAM
#define BOTTOKEN "xxx" 

// bitte auskommentieren falls nicht benutzt
//#define THINGSPEAK
#define THINGSPEAK_KEY "xxx"



// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// INCLUDE SUBROUTINES

#include "c_init.h"
#include "c_median.h"
#include "c_sensor.h"
#include "c_temp.h"
#include "c_fs.h"
#include "c_icons.h"
#include "c_wifi.h"
#include "c_frames.h"
#include "c_bot.h"
#include "c_ota.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++

unsigned long lastUpdateBatteryMode;
unsigned long lastUpdateSensor;
unsigned long lastUpdateCommunication;

void setup() {  

  // Initialize Debug 
  #ifdef DEBUG
    set_serial();
  #endif
  
  // Initialize OLED
  set_OLED();

  // Current Battery Voltage
  get_Vbat();
  
  if (!LADEN) {

    // Open Config-File
    start_fs();
    
    // Initialize Wifi
    set_wifi();

    // Update Time
    if (!isAP)  get_ntp_time();

    // Initialize OTA
    #ifdef OTA  
      set_ota();
      ArduinoOTA.begin();
    #endif
    
    // Initialize Sensors
    set_sensor();
    set_Channels();

    // Initialize Buttons
    set_button();
        
    // Current Wifi Signal Strength
    get_rssi();
  }

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++

void loop() {

  // Lade-Betrieb oder Mess-Betrieb
  if (LADEN) {

    if (!LADENSHOW) {
      drawLoading();
      LADENSHOW = true;
    }
    
    if (millis() - lastUpdateBatteryMode > INTERVALBATTERYMODE) {
      get_Vbat();
      lastUpdateBatteryMode = millis();  

      if (!LADEN) ESP.restart();
    }
    
    return;
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
  int remainingTimeBudget = ui.update();


  if (remainingTimeBudget > 0) {
    // Don't do stuff if you are below your
    // time budget.
    
    if (millis() - lastUpdateSensor > INTERVALSENSOR) {
      get_Vbat();
      get_Temperature();
      
      if (!isAP) {
      if (ch[0].alarm && ch[0].isalarm) {
        
        // Alarmfunktion
        String postStr = "ACHTUNG: ";
        postStr += String(ch[0].temp,1);
      }
      
      }
      lastUpdateSensor = millis();
    }

    if (millis() - lastUpdateCommunication > INTERVALCOMMUNICATION) {
      
      get_rssi();
      
      if (!isAP) {

        #ifdef THINGSPEAK
          sendData();
        #endif

        #ifdef TELEGRAM
          UserData userData;
          getUpdates(id, &userData);
        #endif
      }
      
      lastUpdateCommunication = millis();
    }
    
    //delay(remainingTimeBudget);
  }
}

