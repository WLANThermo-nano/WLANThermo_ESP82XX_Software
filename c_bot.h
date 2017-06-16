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
    
 ****************************************************/

#ifdef THINGSPEAK

  WiFiClient THINGclient;
  #define SERVER1 "api.thingspeak.com"

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Send data to Thingspeak
  void sendData() {

    unsigned long vorher = millis();
  
    // Sendedauer: ~120ms  
    if (THINGclient.connect(SERVER1,80)) {

      String postStr = charts.TSwriteKey;

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

      THINGclient.print("POST /update HTTP/1.1\nHost: api.thingspeak.com\nConnection: close\nX-THINGSPEAKAPIKEY: "
                        +charts.TSwriteKey+"\nContent-Type: application/x-www-form-urlencoded\nContent-Length: "
                        +postStr.length()+"\n\n"+postStr);

      DPRINTF("[INFO]\tSend to Thingspeak: %ums\r\n", millis()-vorher); 
    }

    THINGclient.stop();      
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

#endif



#include <AsyncMqttClient.h>
#define MQTT_HOST "mqtt.thingspeak.com"
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;


void connectToMqtt() {
  DPRINTLN();
  DPRINTP("[INFO]\tWiFi connected to: ");
  DPRINTLN(WiFi.SSID());
  DPRINTP("[INFO]\tIP address: ");
  DPRINTLN(WiFi.localIP());
  DPRINTPLN("[INFO]\tConnecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  connectToMqtt();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DPRINTPLN("[INFO]\tDisconnected from MQTT.");
  if (WiFi.isConnected()) connectToMqtt;
}

void set_mqtt() {
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}


void sendTS() {
    
  if (mqttClient.connected()) {
    
    unsigned long vorher = millis();
    //mqttClient.connect();
    // Sendedauer: ~120ms  

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
  }
}


