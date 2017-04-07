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
  set_serial(); //Serial.setDebugOutput(true);
  
  // Initialize OLED
  set_OLED();

  // Current Battery Voltage
  set_batdetect(false); get_Vbat();
  
  if (!stby) {

    // Open Config-File
    check_ota_sector();
    setEE();start_fs();
    
    // Initialize Wifi
    set_wifi();

    // Update Time
    if (!isAP)  setTime(getNtpTime()); //setSyncProvider(getNtpTime);

    #ifdef DEBUG
      Serial.print("[INFO]\t");
      Serial.println(digitalClockDisplay());
    #endif

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

void loop() {

  // Standby oder Mess-Betrieb
  if (stby) {

    if (!LADENSHOW) {
      drawLoading();
      LADENSHOW = true;
      //wifi_station_disconnect();
      //WiFi.mode(WIFI_OFF);
      pitmaster.active = false;
      // set_pitmaster();
    }
    
    if (millis() - lastUpdateBatteryMode > INTERVALBATTERYMODE) {
      get_Vbat();
      lastUpdateBatteryMode = millis();  

      if (!stby) ESP.restart();
    }
    
    return;
  }

  if (WiFi.status() == WL_CONNECTED & isAP) {
    // Verbindung neu hergestellt, entweder aus AP oder wegen Verbindungsverlust
    isAP = 0;
    #ifdef DEBUG
      Serial.print("[INFO]\tWiFi connected to: ");
      Serial.println(WiFi.SSID());
      Serial.print("[INFO]\tIP address: ");
      Serial.println(WiFi.localIP());
    #endif
    
    WiFi.setAutoReconnect(true); //Automatisch neu verbinden falls getrennt
    disconnectAP = true;
    
  } else if (WiFi.status() != WL_CONNECTED & isAP != 1) {
    // Nicht verbunden
    isAP = 2;
  } else if (isAP == 0 & disconnectAP) {
      uint8_t client_count = wifi_softap_get_station_num();
      if (!client_count) {
        disconnectAP = false;
        WiFi.mode(WIFI_STA);
        #ifdef DEBUG
        Serial.println("[INFO]\tClient hat sich von AP getrennt -> AP abgeschaltet");
        #endif
      }
  }

  //if (beginRequest > 0 & millis()-beginRequest > 4000) {
  //  Serial.println("Senden");
  //  holdRequest->send(200, "text/plain", "Save");
  //}

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
  //server.handleClient();

  pitmaster_control();
  
  // Detect Button Event
  if (button_input()) {
    button_event();
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

      //const char* neuedata[2];
      //neuedata[0] = "WLAN-DDC234";
      //neuedata[1] = "FTZuh5842OBU8753pip";     
      //if (WiFi.status() != WL_CONNECTED) Serial.println(WIFI_Connect(neuedata));
 
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


