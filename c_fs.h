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

    HISTORY:
    0.1.00 - 2016-12-30 initial version
    0.2.00 - 2016-12-30 impliment ChannelData
    
 ****************************************************/

 // HELP: https://github.com/bblanchon/ArduinoJson

#include <FS.h>
#include <ArduinoJson.h>


#define CHANNEL_FILE "/channel.json"
#define WIFI_FILE "/wifi.json"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load Config.json at system start
bool loadConfig() {
  
  File configFile = SPIFFS.open(CHANNEL_FILE, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file");
    #endif
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.println("Config file size is too large");
    #endif
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse config file");
    #endif
    return false;
  }

  const char* author = json["AUTHOR"];

  for (int i=0; i < CHANNELS; i++){
    
    // Fühlertyp auslesen  
    ch[i].typ = json["ttyp"][i];

    // Temperatur MIN auslesen
    ch[i].min = json["tmin"][i];

    // Temperatur MAX auslesen
    ch[i].max = json["tmax"][i];  

    // Temperatur ALARM auslesen
    ch[i].alarm = json["talarm"][i];  
  }

  Serial.print("Loaded Author: ");
  Serial.println(author);

  configFile.close();
  
  #ifdef DEBUG
  json.printTo(Serial);
  Serial.println();
  #endif
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reset config.json to default
bool setConfig() {
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  
  json["AUTHOR"] = "s.ochs";

  JsonArray& _typ = json.createNestedArray("ttyp");
  JsonArray& _min = json.createNestedArray("tmin");
  JsonArray& _max = json.createNestedArray("tmax");
  JsonArray& _alarm = json.createNestedArray("talarm");
  
  for (int i=0; i < CHANNELS; i++){
    _typ.add(2); 
    _min.add(20.0,1);
    _max.add(30.0,1); 
    _alarm.add(false); 
  }
 
  File configFile = SPIFFS.open(CHANNEL_FILE, "w");
  
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file for writing");
    #endif
    return false;
  }

  json.printTo(configFile);    
  configFile.close();

  return true;

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set config.json after Change
bool changeConfig() {

  // Alte Daten auslesen

  File configFile = SPIFFS.open(CHANNEL_FILE, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file");
    #endif
    return false;
  }
  
  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.println("Config file size is too large");
    #endif
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& alt = jsonBuffer.parseObject(buf.get());

  if (!alt.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse current config file");
    #endif
    return false;
  }

  configFile.close();

  // Neue Daten erzeugen

  DynamicJsonBuffer jsonBuffer2;
  JsonObject& neu = jsonBuffer2.createObject();

  if (alt.containsKey("AUTHOR")) {
    neu["AUTHOR"] = alt["AUTHOR"];
  }
  else neu["AUTHOR"] = "neuer Author";

  JsonArray& _typ = neu.createNestedArray("ttyp");
  JsonArray& _min = neu.createNestedArray("tmin");
  JsonArray& _max = neu.createNestedArray("tmax");
  JsonArray& _alarm = neu.createNestedArray("talarm");
    
  for (int i=0; i < CHANNELS; i++){
    _typ.add(ch[i].typ); 
    _min.add(ch[i].min,1);
    _max.add(ch[i].max,1); 
    _alarm.add(ch[i].alarm); 
  }

  // Alte Daten überschreiben
  configFile = SPIFFS.open(CHANNEL_FILE, "w");
  
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file for writing");
    #endif
    return false;
  }
  else Serial.println("Update SPIFFS");

  neu.printTo(configFile);

  #ifdef DEBUG
  neu.printTo(Serial);
  Serial.println();
  #endif
    
  configFile.close();

  return true;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load wifi.json at system start
bool loadWifiSettings() {
  
  File configFile = SPIFFS.open(WIFI_FILE, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open wifi config file");
    #endif
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.println("Wifi config file size is too large");
    #endif
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonArray& json = jsonBuffer.parseArray(buf.get());
  
  if (!json.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse wifi config file");
    #endif
    return false;
  }

  // Wie viele WLAN Schlüssel sind vorhanden
  for (JsonArray::iterator it=json.begin(); it!=json.end(); ++it) {  
    wifissid[lenwifi] = json[lenwifi]["SSID"].asString();
    wifipass[lenwifi] = json[lenwifi]["PASS"].asString();  
    lenwifi++;
  }

  configFile.close();

  #ifdef DEBUG
  json.printTo(Serial);
  Serial.println();
  #endif
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reset config.json to default
bool setWifiSettings() {
  
  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonArray& json = jsonBuffer.createArray();
  
  JsonObject& _wifi1 = json.createNestedObject();

  _wifi1["SSID"] = WIFISSID;
  _wifi1["PASS"] = PASSWORD;
 
  File configFile = SPIFFS.open(WIFI_FILE, "w");
  
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file for writing");
    #endif
    return false;
  }

  json.printTo(configFile);
    
  configFile.close();

  return true;

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Add Wifi Settings to config.json 
bool addWifiSettings(char* ssid, char* pass) {

  // Alte Daten auslesen
  File configFile = SPIFFS.open(WIFI_FILE, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open wifi config file");
    #endif
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.println("Wifi config file size is too large");
    #endif
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonArray& json = jsonBuffer.parseArray(buf.get());
  
  if (!json.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse wifi config file");
    #endif
    return false;
  }

  // Wie viele WLAN Schlüssel sind vorhanden
  int len = 0;
  
  for (JsonArray::iterator it=json.begin(); it!=json.end(); ++it) {  
    len++;
  }

  configFile.close();

  if (len < 5) {

  // Neue Daten eintragen
  JsonObject& _wifi = json.createNestedObject();

  _wifi["SSID"] = ssid;
  _wifi["PASS"] = pass;
 
  configFile = SPIFFS.open(WIFI_FILE, "w");
  
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open config file for writing");
    #endif
    return false;
  }

  json.printTo(configFile);
  
  configFile.close();
  
  return true;
  }

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize FileSystem
void start_fs() {
  
  if (!SPIFFS.begin()) {
    #ifdef DEBUG
    Serial.println("Failed to mount file system");
    #endif
    return;
  }

  //setConfig();

  if (SPIFFS.exists(CHANNEL_FILE)) {
    
    if (!loadConfig()) {
      #ifdef DEBUG
      Serial.println("Failed to load config");
      #endif
    } else {
      #ifdef DEBUG
      Serial.println("Config loaded");
      #endif
    }
  }
  else
    if (!setConfig()) {
      #ifdef DEBUG
      Serial.println("Failed to save config");
      #endif
    } else {
      #ifdef DEBUG
      Serial.println("Config saved");
      #endif
    }

    //setWifiSettings();
    //addWifiSettings(WIFISSID2, PASSWORD2);
    
  if (SPIFFS.exists(WIFI_FILE)) {
    
    if (!loadWifiSettings()) {
      #ifdef DEBUG
      Serial.println("Failed to load wifi config");
      #endif
    } else {
      #ifdef DEBUG
      Serial.println("Wifi config loaded");
      #endif
    }
  }
  else
    if (!setWifiSettings()) {
      #ifdef DEBUG
      Serial.println("Failed to save wifi config");
      #endif
    } else {
      #ifdef DEBUG
      Serial.println("Wifi config saved");
      #endif
    }

}
