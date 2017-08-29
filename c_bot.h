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
static AsyncClient * tssettingclient = NULL;
static AsyncClient * tsdataclient = NULL;           // Telegram Channel Data
static AsyncClient * tsalarmclient = NULL;


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charts
void set_iot(bool init) {
  
   if (init) {
    iot.TS_writeKey = "";     
    iot.TS_httpKey = "";       
    iot.TS_userKey = "";     
    iot.TS_chID = ""; 
    iot.P_MQTT_USER = "";
    iot.P_MQTT_PASS = ""; 
    iot.P_MQTT_QoS = 0;
   }
   
   iot.TS_show8 = false;        
   iot.TS_int = INTERVALCOMMUNICATION/1000;
   iot.TS_on = false;
   iot.P_MQTT_on = false;
   iot.P_MQTT_HOST = "192.168.2.1";
   iot.P_MQTT_PORT = 1883;
   iot.P_MQTT_int = INTERVALCOMMUNICATION/1000;
   iot.TG_on = 0;
   iot.TG_token = "";
   iot.TG_id = "";

   iot.CL_on = false;
   iot.CL_token = newToken();
   iot.CL_int = INTERVALCOMMUNICATION/1000;
      
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Collect Data for Sending to Thingspeak
String collectData() {
  
  String postStr;
  for (int i = 0; i < 7; i++)  {
    if (ch[i].temp != INACTIVEVALUE) {
      postStr += "&";
      postStr += String(i+1);
      postStr += "=";
      postStr += String(ch[i].temp,1);
    }
  }
  if (iot.TS_show8) {
    postStr +="&8=";  
    postStr += String(battery.percentage);  // Kanal 8 ist Batterie-Status
  } else if (ch[7].temp != INACTIVEVALUE) {
    postStr +="&8="; 
    postStr += String(ch[7].temp,1);
  }
  return postStr;
}


String createNote(bool ts) {
  
  String postStr;
  postStr += (ts)?F("&message="):F("&msg=");
  if (ts) postStr += (notification.limit)?F("hoch"):F("niedrig"); 
  else postStr += (notification.limit)?F("up"):F("down");
  postStr += "&ch=";
  postStr += String(notification.ch);

  return postStr;
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
    adress += iot.TS_chID;
    adress += F(".json?api_key=");
    adress += iot.TS_userKey;
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
    printClient(SENDTHINGSPEAK,CLIENTERRROR);
    tsdataclient = NULL;
    delete client;
  }, NULL);

  tsdataclient->onConnect([](void * arg, AsyncClient * client){
    
    tsdataclient->onError(NULL, NULL);

    client->onDisconnect([](void * arg, AsyncClient * c){
      printClient(SENDTHINGSPEAK ,DISCONNECT);
      tsdataclient = NULL;
      delete c;
    }, NULL);

    //send the request
    printClient(SENDTHINGSPEAK,SENDTO);
    String adress = createCommand(POSTMETH,SENDTS,SENDTSLINK,THINGSPEAKSERVER,0);
    client->write(adress.c_str());
    //Serial.println(adress);
        
  }, NULL);

  if(!tsdataclient->connect(THINGSPEAKSERVER, 80)){
    printClient(SENDTHINGSPEAK ,CONNECTFAIL);
    AsyncClient * client = tsdataclient;
    tsdataclient = NULL;
    delete client;
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send Notification to Thingspeak
bool sendNote(int check){

  if (check == 0) {
    if(tsalarmclient) return false;                 //client already exists

    tsalarmclient = new AsyncClient();
    if(!tsalarmclient)  return false;               //could not allocate client

    tsalarmclient->onError([](void * arg, AsyncClient * client, int error){
      printClient(THINGHTTPLINK,CLIENTERRROR);
      tsalarmclient = NULL;
      delete client;
    }, NULL);

  } else if (check == 1) {            // Senden ueber Thingspeak
    
    tsalarmclient->onConnect([](void * arg, AsyncClient * client){
    
      tsalarmclient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        printClient(THINGHTTPLINK ,DISCONNECT);
        tsalarmclient = NULL;
        delete c;
      }, NULL);

      //send the request
      printClient(THINGHTTPLINK,SENDTO);
      String adress = createCommand(GETMETH,THINGHTTP,THINGHTTPLINK,THINGSPEAKSERVER,0);
      client->write(adress.c_str());
      //Serial.println(adress);
    }, NULL);

    if(!tsalarmclient->connect(THINGSPEAKSERVER, 80)){
      printClient(THINGHTTPLINK ,CONNECTFAIL);
      AsyncClient * client = tsalarmclient;
      tsalarmclient = NULL;
      delete client;
    }
    
  } else if (check == 2) {          // Senden ueber Server
    
    tsalarmclient->onConnect([](void * arg, AsyncClient * client){
    
      tsalarmclient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        printClient(SENDNOTELINK ,DISCONNECT);
        tsalarmclient = NULL;
        delete c;
      }, NULL);

      //send the request
      printClient(SENDNOTELINK,SENDTO);
      String adress = createCommand(GETMETH,SENDNOTE,SENDNOTELINK,MESSAGESERVER,0);
      client->write(adress.c_str());
      //Serial.println(adress);
    }, NULL);

    if(!tsalarmclient->connect(MESSAGESERVER, 80)){
      printClient(SENDNOTELINK ,CONNECTFAIL);
      AsyncClient * client = tsalarmclient;
      tsalarmclient = NULL;
      delete client;
    }
  }
  return true;    // Nachricht kann gesendet werden
}


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

