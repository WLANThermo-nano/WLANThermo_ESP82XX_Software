
/***************************************************
 Copyright (C) 2017  Steffen Ochs, Holger Imbery
 
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
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MQTT
void connectToMqtt() {
  DPRINTLN();
  DPRINTP("[INFO]\tWiFi connected to: ");
  DPRINTLN(WiFi.SSID());
  DPRINTP("[INFO]\tIP address: ");
  DPRINTLN(WiFi.localIP());
  DPRINTPLN("[INFO]\tConnecting to MQTT...");
  pmqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  connectToMqtt();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DPRINTPLN("[INFO]\tDisconnected from MQTT.");
  if (WiFi.isConnected()) connectToMqtt;
}

void onMqttConnect(bool sessionPresent) {
  DPRINTPLN("[INFO]\tConnected to MQTT.");
  DPRINTP("[INFO]\tSession present: ");
  DPRINTLN(sessionPresent);
  String adress = F("WLanThermo/");
  adress += sys.host;
  adress += F("/set/#");
  uint16_t packetIdSub = pmqttClient.subscribe(adress.c_str(), 2);
  DPRINTP("[INFO]\tSubscribing at QoS 2, packetId: ");
  DPRINTLN(packetIdSub);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  DPRINTPLN("[INFO]\tSubscribe acknowledged.");
  DPRINTP("[INFO]\t  packetId: ");
  DPRINTLN(packetId);
  DPRINTP("[INFO]\t  qos: ");
  DPRINTLN(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  DPRINTPLN("[INFO]\tUnsubscribe acknowledged.");
  DPRINTP("[INFO]\t  packetId: ");
  DPRINTLN(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
// work in progress - subscribe working, but no action implemented yet
  DPRINTPLN("[INFO]\tPublish received.");
  DPRINTP("[INFO]\t  topic: ");
  DPRINTLN(topic);
  DPRINTP("[INFO]\t  payload: ");
  DPRINTLN(payload);
  DPRINTP("[INFO]\t  qos: ");
  DPRINTLN(properties.qos);
  DPRINTP("[INFO]\t  dup: ");
  DPRINTLN(properties.dup);
  DPRINTP("[INFO]\t  retain: ");
  DPRINTLN(properties.retain);
  DPRINTP("[INFO]\t  len: ");
  DPRINTLN(len);
  DPRINTP("[INFO]\t  index: ");
  DPRINTLN(index);
  DPRINTP("[INFO]\t  total: ");
  DPRINTLN(total);
  String topic_prefix = F("WLanThermo/");
  topic_prefix += sys.host;
  topic_prefix += F("/set/");
  int topic_prefix_length = topic_prefix.length();
  String topic_short = String(topic);
  topic_short.remove(0,topic_prefix_length);
  DPRINTP("[INFO]\t  ActionString: ");
  DPRINTLN(topic_short);
  float new_payload = atof(payload);
  if (topic_short = "temp0/min"){
     ch[0].min = new_payload;
  }
  else if (topic_short = "temp1/min"){
       ch[1].min = new_payload;  
  }
  else if (topic_short = "temp2/min"){
       ch[2].min = new_payload; 
  }
  else if (topic_short = "temp3/min"){
       ch[3].min = new_payload;  
  }
  else if (topic_short = "temp4/min"){
       ch[4].min = new_payload;  
  }
  else if (topic_short = "temp5/min"){
       ch[5].min = new_payload;  
  }
  else if (topic_short = "temp6/min"){
       ch[6].min = new_payload;
  }
  else if (topic_short = "temp7/min"){
       ch[7].min = new_payload;
  }
  else if (topic_short = "temp0/max"){
       ch[0].max = new_payload;
  }
  else if (topic_short = "temp1/max"){
       ch[1].max = new_payload;
  }
  else if (topic_short = "temp2/max"){
       ch[2].max = new_payload;
  }
  else if (topic_short = "temp3/max"){
       ch[3].max = new_payload;
  }
  else if (topic_short = "temp4/max"){
       ch[4].max = new_payload;
  }
  else if (topic_short = "temp5/max"){
       ch[5].max = new_payload;
  }
  else if (topic_short = "temp6/max"){
       ch[6].max = new_payload;
  }
  else if (topic_short = "temp7/max"){
       ch[7].max = new_payload;
  }
  setconfig(eCHANNEL,{});  
}

void set_pmqtt() {
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  pmqttClient.onConnect(onMqttConnect);
  pmqttClient.onDisconnect(onMqttDisconnect);
  pmqttClient.onSubscribe(onMqttSubscribe);
  pmqttClient.onUnsubscribe(onMqttUnsubscribe);
  pmqttClient.onMessage(onMqttMessage);
  pmqttClient.setServer(charts.P_MQTT_HOST.c_str(), charts.P_MQTT_PORT);
  pmqttClient.setCredentials(charts.P_MQTT_USER.c_str(), charts.P_MQTT_PASS.c_str());
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Send data to private MQTT Broker
void sendpmqtt() {
     
    if (pmqttClient.connected()) {
        
        unsigned long vorher = millis();
        
        for (int i = 0; i < 8; i++)  {
            if (ch[i].temp != INACTIVEVALUE) {
                String adress = F("WLanThermo/");
                adress += sys.host;
                adress += F("/status/");
                adress += "temp";
                adress += String(i+1);
                String postStr = String(ch[i].temp,1);
                pmqttClient.publish(adress.c_str(), charts.P_MQTT_QoS, false, postStr.c_str());
            }
        }
        
        String adress = F("WLanThermo/");
        adress += sys.host;
        adress += F("/status/");
        adress += "voltage";
        String postStr = String(battery.percentage);
        pmqttClient.publish(adress.c_str(), charts.P_MQTT_QoS, false, postStr.c_str());
        
        DPRINTF("[INFO]\tPublish to MQTTbroker: %ums\r\n", millis()-vorher);
        
    } else {
      DPRINTPLN("[INFO]\tNot Connected to MQTT Broker");
      pmqttClient.connect();
    }
}

