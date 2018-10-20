 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

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

    LITERATUR:
    - https://de.wikipedia.org/wiki/Liste_der_HTTP-Headerfelder
    
 ****************************************************/


// see: https://github.com/me-no-dev/ESPAsyncTCP/issues/18
static AsyncClient * tsalarmclient = NULL;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize IoT
void set_iot(bool init) {
  
   if (init) {                // clear all
    iot.TS_writeKey = "";     
    iot.TS_httpKey = "";       
    iot.TS_userKey = "";     
    iot.TS_chID = ""; 
    
    iot.P_MQTT_USER = "";
    iot.P_MQTT_PASS = ""; 
    iot.P_MQTT_QoS = 0;
   }
   
   iot.TS_show8 = false;        
   iot.TS_int = INTERVALCOMMUNICATION/4;
   iot.TS_on = false;
   
   iot.P_MQTT_on = false;
   iot.P_MQTT_HOST = "192.168.2.1";
   iot.P_MQTT_PORT = 1883;
   iot.P_MQTT_int = INTERVALCOMMUNICATION/4;
   
   iot.CL_on = false;
   iot.CL_token = newToken();
   iot.CL_int = INTERVALCOMMUNICATION/4;
}

void set_push() {
  pushd.on = 0;
  pushd.token = "";
  pushd.id = "";
  pushd.repeat = 1;
  pushd.service = 0;
}

// im Umbau
void sendNotification() {
  

    if (notification.type > 0) {                      // GENERAL NOTIFICATION       
        
      if (pushd.on > 0) {
        if (sendAPI(0)) {
          apiindex = APINOTE;
          urlindex = NOTELINK;
          sendAPI(2);           // Notification per Nano-Server
        }
      }
        
    } else if (notification.index > 0) {              // CHANNEL NOTIFICATION

      for (int i=0; i < CHANNELS; i++) {
        if (notification.index & (1<<i)) {            // ALARM AT CHANNEL i
            
          bool sendN = true;
          if (pushd.on > 0) {
            if (sendAPI(0)) {
              notification.ch = i;
              apiindex = APINOTE;
              urlindex = NOTELINK;
              sendAPI(2);           // Notification per Nano-Server
            } else sendN = false;
          }
          if (sendN) {
            notification.index &= ~(1<<i);           // Kanal entfernen, sonst erneuter Aufruf
            return;                                  // nur ein Senden pro Durchlauf
          }
        }
      }    
    }
  

  if (pushd.on == 3) loadconfig(ePUSH,0);     // nach Testnachricht alte Werte wieder herstellen
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


