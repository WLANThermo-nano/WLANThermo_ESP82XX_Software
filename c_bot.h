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


//WiFiClient THINGclient;
#define SERVER1 "api.thingspeak.com"

// see: https://github.com/me-no-dev/ESPAsyncTCP/issues/18
static AsyncClient * tssettingclient = NULL;
static AsyncClient * tsdataclient = NULL;
static AsyncClient * tsalarmclient = NULL;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charts
void set_charts(bool init) {
  
   if (init) {
    charts.TS_writeKey = "";     
    charts.TS_httpKey = "";       
    charts.TS_userKey = ""; //"Q2EID9PNX0YQVGRH";       
    charts.TS_chID = ""; 
    charts.P_MQTT_USER = "";
    charts.P_MQTT_PASS = ""; 
    charts.P_MQTT_QoS = 0;
    
   }
   
   charts.TS_show8 = false;        
   charts.TS_int = INTERVALCOMMUNICATION/1000;
   charts.TS_on = true;
   charts.P_MQTT_on = false;
   charts.P_MQTT_HOST = "192.168.2.1";
   charts.P_MQTT_PORT = 1883;
   charts.TG_on = false;
   charts.TG_token = "";
   charts.TG_id = "";
      
}

  
/*
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Settings to Thingspeak
void sendSettings(){

  if(tssettingclient) return;                 //client already exists

  tssettingclient = new AsyncClient();
  if(!tssettingclient)  return;               //could not allocate client

  tssettingclient->onError([](void * arg, AsyncClient * client, int error){
    Serial.println("[INFO]\tConnect Error");
    tssettingclient = NULL;
    delete client;
  }, NULL);

  tssettingclient->onConnect([](void * arg, AsyncClient * client){
    
    Serial.println(millis());
    
    tssettingclient->onError(NULL, NULL);

    client->onDisconnect([](void * arg, AsyncClient * c){
      Serial.println(millis());
      tssettingclient = NULL;
      delete c;
    }, NULL);

    client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
      Serial.print("\r\nData: ");
      Serial.println(len);
      uint8_t * d = (uint8_t*)data;
      for(size_t i=0; i<len;i++)
        Serial.write(d[i]);
    }, NULL);

    //send the request
    String postStr;
    
    AsyncWebServerRequest *request;
    
    // Metadata
    postStr = "&metadata=";
    //postStr += handleSettings(request, 2);
    
    String adress = F("PUT /channels/");
    adress += charts.TS_chID;
    adress += F(".json?api_key=");
    adress += charts.TS_userKey;
    adress += postStr;
    adress += F(" HTTP/1.1\nHost: api.thingspeak.com\n\n");
    
    client->write(adress.c_str());
    
    
  }, NULL);

  if(!tssettingclient->connect(SERVER1, 80)){
    Serial.println("[INFO]\tConnect Fail");
    AsyncClient * client = tssettingclient;
    tssettingclient = NULL;
    delete client;
  }
}

*/

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Temp-Data to Thingspeak
void sendDataTS(){

  if(tsdataclient) return;                 //client already exists

  tsdataclient = new AsyncClient();
  if(!tsdataclient)  return;               //could not allocate client

  tsdataclient->onError([](void * arg, AsyncClient * client, int error){
    DPRINTPLN("[INFO]\tThingspeak Data Client Connect Error");
    tsdataclient = NULL;
    delete client;
  }, NULL);

  tsdataclient->onConnect([](void * arg, AsyncClient * client){
    
    tsdataclient->onError(NULL, NULL);

    client->onDisconnect([](void * arg, AsyncClient * c){
      tsdataclient = NULL;
      delete c;
    }, NULL);

    //send the request
    String postStr;

    for (int i = 0; i < 7; i++)  {
      if (ch[i].temp != INACTIVEVALUE) {
        postStr += "&";
        postStr += String(i+1);
        postStr += "=";
        postStr += String(ch[i].temp,1);
      }
    }

    if (charts.TS_show8) {
      postStr +="&8=";  
      postStr += String(battery.percentage);  // Kanal 8 ist Batterie-Status
    } else if (ch[7].temp != INACTIVEVALUE) {
      postStr +="&8="; 
      postStr += String(ch[7].temp,1);
    }

    AsyncWebServerRequest *request;
    //postStr = "&status=";
    //postStr += handleData(request, 2);
    
    String adress = F("POST /update.json?api_key=");
    adress += charts.TS_writeKey;
    adress += postStr;          // starts with &
    adress += F(" HTTP/1.1\nHost: api.thingspeak.com\n\n");

    DPRINTPLN("[INFO]\tSend to Thingspeak"); 
    client->write(adress.c_str());
        
  }, NULL);

  if(!tsdataclient->connect(SERVER1, 80)){
    Serial.println("[INFO]\tThingspeak Server Connect Fail");
    AsyncClient * client = tsdataclient;
    tsdataclient = NULL;
    delete client;
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Temp-Data to Thingspeak
bool sendMessage(bool check){

  if (check) {
    if(tsalarmclient) return false;                 //client already exists

    tsalarmclient = new AsyncClient();
    if(!tsalarmclient)  return false;               //could not allocate client

    return true;                                    // Nachricht kann gesendet werden

  } else {
    
    tsalarmclient->onError([](void * arg, AsyncClient * client, int error){
      DPRINTPLN("[INFO]\tThingspeak Alarm Client Connect Error");
      tsalarmclient = NULL;
      delete client;
    }, NULL);

    tsalarmclient->onConnect([](void * arg, AsyncClient * client){
    
      tsalarmclient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        tsalarmclient = NULL;
        delete c;
      }, NULL);

      //send the request

      String postStr = "&message=";
    
      if (notification.limit) 
        postStr += "hoch";
      else 
        postStr += "niedrig";
      postStr += "&ch=";
      postStr += String(notification.ch);
    
      String adress = F("GET /apps/thinghttp/send_request?api_key=");
      adress += charts.TS_httpKey;
      adress += postStr;          // starts with &
      adress += F(" HTTP/1.1\nHost: api.thingspeak.com\n\n");

      DPRINTPLN("[INFO]\tSend Alarm to Thingspeak"); 
      client->write(adress.c_str());
        
    }, NULL);

    if(!tsalarmclient->connect(SERVER1, 80)){
      Serial.println("[INFO]\tThingspeak Server Connect Fail");
      AsyncClient * client = tsalarmclient;
      tsalarmclient = NULL;
      delete client;
    }

    return true;
  }
}

/*
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send Message to Telegram via Thingspeak
  void sendMessage(int ch, int count) {

    unsigned long vorher = millis();

    // Sendedauer: ~120 ms
    if (THINGclient.connect(SERVER1,80)) {
 
      String url = "/apps/thinghttp/send_request?api_key=";
      url += charts.TS_httpKey;
      url += "&message=";
      if (count) url += "hoch";
      else url += "niedrig";
      url += "&ch=";
      url += String(ch);
    
      THINGclient.print("GET " + url + " HTTP/1.1\r\n" + "Host: " + SERVER1 
                        + "\r\n" + "Connection: close\r\n\r\n");
  
      DPRINTF("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    }

    THINGclient.stop(); 
  }
*/

/*
#include "include/ssl.h"
static AsyncClient * aClient = NULL;

#define SERVER2 "192.168.254.16"
const char *sslHost = "192.168.254.16";
const uint16_t sslPort = 443;

void runAsyncClient(){
  if(aClient)//client already exists
    return;

  aClient = new AsyncClient();
  if(!aClient)//could not allocate client
    return;

  aClient->onError([](void * arg, AsyncClient * client, int error){
    Serial.println("Connect Error");
    aClient = NULL;
    delete client;
  }, NULL);

  aClient->onConnect([](void * arg, AsyncClient * client){
    Serial.println("Connected");
    //securePrintInfo(client->getSSL());
    aClient->onError(NULL, NULL);

    client->onDisconnect([](void * arg, AsyncClient * c){
      Serial.println("Disconnected");
      aClient = NULL;
      delete c;
    }, NULL);

    client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
      Serial.print("\r\nData: ");
      Serial.println(len);
      uint8_t * d = (uint8_t*)data;
      for(size_t i=0; i<len;i++)
        Serial.write(d[i]);
    }, NULL);

    //send the request
    char m[256];
    sprintf(m, "GET /test.htm HTTP/1.0\r\nHost: %s\r\n\r\n", sslHost);
    int wrote = client->write(m, strlen(m));
    Serial.printf("Sent: %u => %d\r\n", strlen(m), wrote);
  }, NULL);

  if(!aClient->connect(SERVER2, 443, true)){
    Serial.println("Connect Fail");
    AsyncClient * client = aClient;
    aClient = NULL;
    delete client;
  }
}
*/

// https://api.telegram.org/bot280220123:AAHdw_5QeO1lfIhXU8ja_IlpSSg2gYTJXtU/sendMessage
//chat_id=256288661&text=ACHTUNG:+Kanal+%%ch%%+ist+zu+%%message%%

