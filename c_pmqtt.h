
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


//#define MQTT_DEBUG              // ENABLE SERIAL MQTT DEBUG MESSAGES

#ifdef MQTT_DEBUG
  #define MQPRINT(...)    Serial.print(__VA_ARGS__)
  #define MQPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define MQPRINTP(...)   Serial.print(F(__VA_ARGS__))
  #define MQPRINTPLN(...) Serial.println(F(__VA_ARGS__))
  #define MQPRINTF(...)   Serial.printf(__VA_ARGS__)
  
#else
  #define MQPRINT(...)     //blank line
  #define MQPRINTLN(...)   //blank line 
  #define MQPRINTP(...)    //blank line
  #define MQPRINTPLN(...)  //blank line
  #define MQPRINTF(...)    //blank line
#endif
 
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Start MQTT
void connectToMqtt() {
  if (iot.P_MQTT_on) {
    set_pmqtt();
    pmqttClient.connect();
    wifi.mqttreconnect = millis();
    IPRINTPLN("t:MQTT");
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MQTT Handler
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  IPRINTPLN("d:MQTT");
  wifi.mqttreconnect = millis();    // Reconnect initialisieren
  sys.sendSettingsflag = false;
}

void onMqttConnect(bool sessionPresent) {
  IPRINTPLN("c:MQTT");
  wifi.mqttreconnect = 0;
  MQPRINTP("[MQTT]\tSession present: ");
  MQPRINTLN(sessionPresent);
  String adress = F("WLanThermo/");
  adress += sys.host;
  adress += F("/#");
  uint16_t packetIdSub = pmqttClient.subscribe(adress.c_str(), 2);
  MQPRINTP("[MQTT]\tSubscribing at QoS 2, packetId: ");
  MQPRINTLN(packetIdSub);
  //pmqttClient.publish("WLanThermo/NANO-850e43/status/data", 0, false, "test 1");
  sys.sendSettingsflag = true;
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  MQPRINTPLN("[MQTT]\tSubscribe acknowledged.");
  MQPRINTP("packetId: ");
  MQPRINTLN(packetId);
  MQPRINTP("qos: ");
  MQPRINTLN(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  MQPRINTPLN("[MQTT]\tUnsubscribe acknowledged.");
  MQPRINTP("packetId: ");
  MQPRINTLN(packetId);
}

void onMqttMessage(char* topic, char* datas, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String topic_prefix = F("WLanThermo/");
  topic_prefix += sys.host;
  int topic_prefix_length = topic_prefix.length();
  String topic_short = String(topic);
  topic_short.remove(0, topic_prefix_length);

  if (topic_short.startsWith("/set/channels")) {
    bodyWebHandler.setChannels((uint8_t*) datas);
  }
  if (topic_short.startsWith("/set/system")) {
    bodyWebHandler.setSystem((uint8_t*) datas);
  } 
  if (topic_short.startsWith("/set/pitmaster")) {
    bodyWebHandler.setPitmaster((uint8_t*) datas);
  } 
  if (topic_short.startsWith("/set/pid")) {
    bodyWebHandler.setPID((uint8_t*) datas);
  }  
  if (topic_short.startsWith("/set/iot")) {
    bodyWebHandler.setIoT((uint8_t*) datas);
  }
  if (topic_short.startsWith("/get/settings")) {
    sendSettings();
  }
  if (topic_short.startsWith("/get/data")) {
    sendpmqtt();
  }
  // placeholder for future extensions
  // if (topic_short.startsWith("/cmd/action")) {
  // dummy_action_handler();
  //}
}

void onMqttPublish(uint16_t packetId) {
  MQPRINTPLN("[MQTT]\tPublish acknowledged.");
  MQPRINTP("  packetId: ");
  MQPRINTLN(packetId);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Configuration MQTT
void set_pmqtt(bool ini) {

  if (ini) {
    pmqttClient.onConnect(onMqttConnect);
    pmqttClient.onDisconnect(onMqttDisconnect);
    pmqttClient.onSubscribe(onMqttSubscribe);
    pmqttClient.onUnsubscribe(onMqttUnsubscribe);
    pmqttClient.onMessage(onMqttMessage);
    pmqttClient.onPublish(onMqttPublish);
  }
  pmqttClient.setServer(iot.P_MQTT_HOST.c_str(), iot.P_MQTT_PORT);
  if (iot.P_MQTT_USER != "" && iot.P_MQTT_PASS != "")
    pmqttClient.setCredentials(iot.P_MQTT_USER.c_str(), iot.P_MQTT_PASS.c_str());
}

String prefixgen(uint8_t stil = 0) {
  String prefix = F("WLanThermo/");
  prefix += sys.host;

  switch (stil) {
    case 1: return prefix + F("/status/data");
    case 2: return prefix + F("/status/settings");
    default: return prefix + F("/#");
  }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// send datas
bool sendpmqtt() {

  if (pmqttClient.connected()) {
    String payload_data = apiData(APIDATA);
    pmqttClient.publish(prefixgen(1).c_str(), iot.P_MQTT_QoS, false, payload_data.c_str());
    return true;

  } else {
    return false;
  }
}
// send settings
bool sendSettings() {
  
    if (pmqttClient.connected()) {
      String payload_settings = apiData(APISETTINGS);
      pmqttClient.publish(prefixgen(2).c_str(), iot.P_MQTT_QoS, false, payload_settings.c_str());
      return true;
  
    } else {
      return false;
    }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++
// control mqtt 
void checkMqtt() {
  // Anschalten erfolgt innerhalb des Wifi-Prozesses, hier nur Abschalten und Daten initialisieren
  if (!iot.P_MQTT_on && pmqttClient.connected()) pmqttClient.disconnect();                      // Abschalten
  else if (sys.sendSettingsflag) {                                                              // Daten initalizieren
    if (sendpmqtt() && sendSettings()) sys.sendSettingsflag = false;
  }
}  
