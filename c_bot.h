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


WiFiClient THINGclient;
#define SERVER1 "api.thingspeak.com"

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charts
void set_charts(bool init) {
  
   if (init) {
    charts.TSwriteKey = "";     
    charts.TShttpKey = "";       
    charts.TSuserKey = ""; //"Q2EID9PNX0YQVGRH";       
    charts.TSchID = ""; 
   }
   
   charts.TSshow8 = false;        
   charts.TSint = INTERVALCOMMUNICATION/1000;
   charts.TSon = true;   
}

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send data to Thingspeak
  void sendTS() {
    
    if (mqttClient.connected()) {
    
      unsigned long vorher = millis();
      //mqttClient.connect();
      // Sendedauer: ~1ms  

      String postStr = "";

      for (int i = 0; i < 7; i++)  {
        if (ch[i].temp != INACTIVEVALUE) {
          postStr += "&";
          postStr += String(i+1);
          postStr += "=";
          postStr += String(ch[i].temp,1);
        }
      }

      if (charts.TSshow8) {
        postStr +="&8=";  
        postStr += String(battery.percentage);  // Kanal 8 ist Batterie-Status
      } else if (ch[7].temp != INACTIVEVALUE) {
        postStr +="&8="; 
        postStr += String(ch[7].temp,1);
      }

      String adress = F("channels/");
      adress += charts.TSchID;
      adress += F("/publish/");
      adress += charts.TSwriteKey;

      mqttClient.publish(adress.c_str(), 0, false, postStr.c_str());
    
      //mqttClient.disconnect();
      DPRINTF("[INFO]\tPublish to Thingspeak at QoS 0: %ums\r\n", millis()-vorher); 
    } else mqttClient.connect();
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send Message to Telegram via Thingspeak
  void sendMessage(int ch, int count) {

    unsigned long vorher = millis();

    // Sendedauer: ~120 ms
    if (THINGclient.connect(SERVER1,80)) {
 
      String url = "/apps/thinghttp/send_request?api_key=";
      url += charts.TShttpKey;
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


// see: https://github.com/me-no-dev/ESPAsyncTCP/issues/18
static AsyncClient * aClient = NULL;
static AsyncClient * tsdataclient = NULL;

void sendSettings(){

  if(aClient) return;                 //client already exists

  aClient = new AsyncClient();
  if(!aClient)  return;               //could not allocate client

  aClient->onError([](void * arg, AsyncClient * client, int error){
    Serial.println("[INFO]\tConnect Error");
    aClient = NULL;
    delete client;
  }, NULL);

  aClient->onConnect([](void * arg, AsyncClient * client){
    
    Serial.println(millis());
    
    aClient->onError(NULL, NULL);

    client->onDisconnect([](void * arg, AsyncClient * c){
      Serial.println(millis());
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
    String postStr;
    
    AsyncWebServerRequest *request;
    
    // Metadata
    postStr = "&metadata=";
    postStr += handleSettings(request, 2);
    
    String adress = F("PUT /channels/");
    adress += charts.TSchID;
    adress += F(".json?api_key=");
    adress += charts.TSuserKey;
    adress += postStr;
    adress += F(" HTTP/1.1\nHost: api.thingspeak.com\n\n");
    
    client->write(adress.c_str());
    
    
  }, NULL);

  if(!aClient->connect(SERVER1, 80)){
    Serial.println("[INFO]\tConnect Fail");
    AsyncClient * client = aClient;
    aClient = NULL;
    delete client;
  }
}



void sendDataTS(){

  if(tsdataclient) return;                 //client already exists

  tsdataclient = new AsyncClient();
  if(!tsdataclient)  return;               //could not allocate client

  tsdataclient->onError([](void * arg, AsyncClient * client, int error){
    DPRINTPLN("[INFO]\tThingspeak Client Connect Error");
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

    if (charts.TSshow8) {
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
    adress += charts.TSwriteKey;
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



