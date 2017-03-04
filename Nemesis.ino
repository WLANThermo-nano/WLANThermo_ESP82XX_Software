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

// Entwicklereinstellungen
#define OTA                                 // ENABLE OTA UPDATE
#define DEBUG                               // ENABLE SERIAL DEBUG MESSAGES
#define THINGSPEAK
//#define KTYPE
bool isEco = false;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++
// INCLUDE SUBROUTINES

#include "c_init.h"
#include "c_median.h"
#include "c_sensor.h"
#include "c_pitmaster.h"
#include "c_temp.h"
#include "c_fs.h"
#include "c_icons.h"
#include "c_wifi.h"
#include "c_frames.h"
#include "c_bot.h"
#include "c_ota.h"
#include "c_server.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++

unsigned long lastUpdateBatteryMode;
unsigned long lastUpdateSensor;
unsigned long lastUpdateCommunication;
unsigned long lastUpdateDatalog;
unsigned long lastFlashInWork;

void setup() {  

  // Initialize Serial 
  set_serial();
  
  // Initialize OLED
  set_OLED();

  // Current Battery Voltage
  set_batdetect(); get_Vbat();
  
  if (!stby) {

    // Open Config-File
    check_ota_sector();
    setEE();start_fs();
    
    // Initialize Wifi
    set_wifi();

    // Update Time
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

void loop() {

  // Standby oder Mess-Betrieb
  if (stby) {

    if (!LADENSHOW) {
      drawLoading();
      LADENSHOW = true;
      //wifi_station_disconnect();
      //WiFi.mode(WIFI_OFF);
      // set_pitmaster();
    }
    
    if (millis() - lastUpdateBatteryMode > INTERVALBATTERYMODE) {
      get_Vbat();
      lastUpdateBatteryMode = millis();  

      if (!stby) ESP.restart();
    }
    
    return;
  }

  // Detect Serial
  static char serialbuffer[110];
  if (readline(Serial.read(), serialbuffer, 110) > 0) {
    read_serial(serialbuffer);
  }
  
  // Detect OTA
  #ifdef OTA
    ArduinoOTA.handle();
  #endif

  // Server
  server.handleClient();

  pitmaster_control();
  
  // Detect Button Event
  if (button_input()) {
    button_event();
  }

  if (awaking) {
    check_wifi();
    //monitorWiFi();
  }
  
  // Update Display
  int remainingTimeBudget;
  if (!displayblocked) {
    remainingTimeBudget = ui.update();
  } else remainingTimeBudget = 1;


  if (remainingTimeBudget > 0) {
    // Don't do stuff if you are below your
    // time budget.
    
    if (millis() - lastUpdateSensor > INTERVALSENSOR) {
      
      get_Temperature();
      get_Vbat();

      controlAlarm(0);
      lastUpdateSensor = millis();
    
    }

    if (millis() - lastUpdateCommunication > INTERVALCOMMUNICATION) {

      get_rssi(); // müsste noch an einen anderen Ort wo es unabhängig von INTERVALCOM.. ist
      cal_soc();
      
      // Erst aufwachen falls im EcoModus
      // UpdateCommunication wird so lange wiederholt bis ESP wieder wach
      if (isEco && !awaking && (WiFi.status() != WL_CONNECTED)) {
        reconnect_wifi();
      }
      else if (!awaking) {

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

        // Wieder einschlafen
        if (isEco) {
          stop_wifi();
        }
      
        lastUpdateCommunication = millis();
      }
      
    }

    if (millis() - lastUpdateDatalog > 5000) {
      
      for (int i=0; i < CHANNELS; i++)  {
        mylog[log_count].tem[i] = (uint16_t) (ch[i].temp * 10);
      }
      mylog[log_count].timestamp = now();

      if (log_count < MAXLOGCOUNT-1) {
        log_count++;
      } else {
        log_count = 0;
        /*write_flash(log_sector);
        read_flash(log_sector);
        log_sector++;
        
        // Test
        
        for (int j=0; j<10; j++) {
          int16_t test = archivlog[j].tem[0];
          Serial.print(test/10.0);
          Serial.print(" ");
        }
        Serial.println();
        */
      }

      // TEST
      //Serial.println(ulMeasCount%ulNoMeasValues);
      //ulMeasCount++;

      lastUpdateDatalog = millis();
    }

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


