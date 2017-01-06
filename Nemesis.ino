<<<<<<< HEAD
/*************************************************** 
    Copyright (C) 2016  Steffen Ochs
=======
 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs, Phantomias2006
>>>>>>> refs/remotes/origin/master

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
<<<<<<< HEAD
=======
    0.2.01 - 2017-01-02 Change Button Event
>>>>>>> refs/remotes/origin/master
    
 ****************************************************/


// CHOOSE CONFIGURATION (user input)

// bitte auskommentieren falls nicht benutzt
#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES

// bitte nicht zutreffendes auskommentieren
//#define VARIANT_A                           // 3xNTC// CHOOSE HARDWARE
#define VARIANT_B                           // 6xNTC, 1xSYSTEM
//#define VARIANT_C                           // 4xNTC, 1xKYTPE, 1xSYSTEM

<<<<<<< HEAD
// falls erstes Flashen "xxx" ersetzen (nicht auskommentieren)
=======
// falls erstes Flashen "xxx" ersetzen
>>>>>>> refs/remotes/origin/master
#define WIFISSID "xxx"              // SET WIFI SSID (falls noch kein wifi.json angelegt ist)  
#define PASSWORD "xxx"              // SET WIFI PASSWORD (falls noch kein wifi.json angelegt ist)

// bitte auskommentieren falls nicht benutzt
<<<<<<< HEAD
#define TELEGRAM
#define BOTTOKEN "xxx" 

// bitte auskommentieren falls nicht benutzt
#define THINGSPEAK
=======
//#define TELEGRAM
#define BOTTOKEN "xxx" 

// bitte auskommentieren falls nicht benutzt
//#define THINGSPEAK
>>>>>>> refs/remotes/origin/master
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
<<<<<<< HEAD

=======
#include "c_server.h"
>>>>>>> refs/remotes/origin/master

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
<<<<<<< HEAD
    if (!isAP)  get_ntp_time();

    // Initialize OTA
    #ifdef OTA  
    set_ota();
    ArduinoOTA.begin();
=======
    if (!isAP)  setTime(getNtpTime()); //setSyncProvider(getNtpTime);

    #ifdef DEBUG
    digitalClockDisplay();
    #endif
   
    // Initialize Server
    server_setup();

    // Initialize OTA
    #ifdef OTA  
      set_ota();
      ArduinoOTA.begin();
>>>>>>> refs/remotes/origin/master
    #endif
    
    // Initialize Sensors
    set_sensor();
    set_Channels();

    // Initialize Buttons
    set_button();
        
    // Current Wifi Signal Strength
    get_rssi();
<<<<<<< HEAD

=======
>>>>>>> refs/remotes/origin/master
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
<<<<<<< HEAD

  // Detect OTA
  #ifdef OTA
  ArduinoOTA.handle();
  #endif
  
  // Detect Button Event
  button_get();

  // Update Display
  int remainingTimeBudget = ui.update();
=======
 
  // Detect Serial
  serialEvent();
  if (receiveSerial) read_serial();

  // Detect OTA
  #ifdef OTA
    ArduinoOTA.handle();
  #endif
 
 // Server
  server.handleClient();
  
  // Detect Button Event
  if (button_input()) {
    button_event();
  }

    // Update Display
  int remainingTimeBudget;
  if (!displayblocked) {
    remainingTimeBudget = ui.update();
  } else remainingTimeBudget = 1;
>>>>>>> refs/remotes/origin/master


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
<<<<<<< HEAD
      }  
=======
      }
>>>>>>> refs/remotes/origin/master
      
      }
      lastUpdateSensor = millis();
    }

    if (millis() - lastUpdateCommunication > INTERVALCOMMUNICATION) {
      
      get_rssi();
      
      if (!isAP) {

        #ifdef THINGSPEAK
<<<<<<< HEAD
        sendData();
        #endif

        #ifdef TELEGRAM
        UserData userData;
        getUpdates(id, &userData);
=======
          sendData();
        #endif

        #ifdef TELEGRAM
          UserData userData;
          getUpdates(id, &userData);
>>>>>>> refs/remotes/origin/master
        #endif
      }
      
      lastUpdateCommunication = millis();
    }
    
    //delay(remainingTimeBudget);
  }
}

