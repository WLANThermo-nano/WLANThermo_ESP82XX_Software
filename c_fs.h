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
    0.2.01 - 2017-01-04 add version and temp_unit in channel.json
    0.2.02 - 2017-01-05 add serial communication
    0.2.03 - 2017-01-20 add Thingspeak config
    
 ****************************************************/

 // HELP: https://github.com/bblanchon/ArduinoJson

#include <FS.h>
#include <ArduinoJson.h>


#define CHANNEL_FILE "/channel.json"
#define WIFI_FILE "/wifi.json"
#define THING_FILE "/thing.json"


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
  
  configFile.close();

  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse config file");
    #endif
    return false;
  }
  
  #ifdef DEBUG
  json.printTo(Serial);
  Serial.println();
  #endif

  int _version = json["VERSION"];

  if (_version == CHANNELJSONVERSION) {
  
    const char* author = json["AUTHOR"];
    temp_unit = json["temp_unit"].asString();

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

    return true;
  
  }

  // Falsche Version
  return false;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reset config.json to default
bool setConfig() {
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  
  json["AUTHOR"] = "s.ochs";
  json["VERSION"] = CHANNELJSONVERSION;
  json["temp_unit"] = temp_unit;

  JsonArray& _typ = json.createNestedArray("ttyp");
  JsonArray& _min = json.createNestedArray("tmin");
  JsonArray& _max = json.createNestedArray("tmax");
  JsonArray& _alarm = json.createNestedArray("talarm");
  
  for (int i=0; i < CHANNELS; i++){
    _typ.add(2);
    
    if (temp_unit == "F") {
      _min.add(68.0,1);
      _max.add(86.0,1);
    } else {
      _min.add(20.0,1);
      _max.add(30.0,1); 
    }
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

  neu["AUTHOR"] = alt["AUTHOR"];
  neu["VERSION"] = CHANNELJSONVERSION;
  neu["temp_unit"] = temp_unit;

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
    Serial.println("[INFO]\tFailed to open channel config file for writing");
    #endif
    return false;
  }
  else Serial.println("[INFO]\tUpdate Channel Config");

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
// Reset wifi.json to default
bool setWifiSettings(const char* ssid, const char* pass) {
  
  //StaticJsonBuffer<200> jsonBuffer;
  DynamicJsonBuffer jsonBuffer;
  JsonArray& json = jsonBuffer.createArray();
  
  JsonObject& _wifi1 = json.createNestedObject();

  _wifi1["SSID"] = ssid;
  _wifi1["PASS"] = pass;
 
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
// Add Wifi Settings to wifi.json 
bool addWifiSettings(const char* ssid, const char* pass) {

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

  #ifdef DEBUG
  json.printTo(Serial);
  Serial.println();
  #endif
  
  
  return true;
  }

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load thing.json at system start
bool loadThingSettings() {
  
  File configFile = SPIFFS.open(THING_FILE, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.println("Failed to open Thingspeak config file");
    #endif
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.println("Thingspeak config file size is too large");
    #endif
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  
  if (!json.success()) {
    #ifdef DEBUG
    Serial.println("Failed to parse Thingspeak config file");
    #endif
    return false;
  }

  THINGSPEAK_KEY = json["KEY"].asString();
  
  configFile.close();

  #ifdef DEBUG
  json.printTo(Serial);
  Serial.println();
  #endif
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set thing.json
bool setThingSettings(const char* key) {
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  
  THINGSPEAK_KEY = key;
  json["KEY"] = THINGSPEAK_KEY;
  
  File configFile = SPIFFS.open(THING_FILE, "w");
  
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
// Initialize FileSystem
void start_fs() {
  
  if (!SPIFFS.begin()) {
    #ifdef DEBUG
    Serial.println("[INFO]\tFailed to mount file system");
    #endif
    return;
  }

  /*
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  */
  

  if (SPIFFS.exists(CHANNEL_FILE)) {
    
    if (!loadConfig()) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to load config");
      #endif

      // Falsche Version ueberschreiben
      if (!setConfig()) {
        #ifdef DEBUG
          Serial.println("[INFO]\tFailed to save config");
        #endif
      } else {
        #ifdef DEBUG
          Serial.println("[INFO]\tChannel config saved");
        #endif
        ESP.restart();
      }
      
    } else {
      #ifdef DEBUG
      Serial.println("[INFO]\tChannel config loaded");
      #endif
    }
  }
  else
    if (!setConfig()) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save channel config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tChannel config saved");
      #endif
      ESP.restart();
    }

    //setWifiSettings();
    //addWifiSettings(WIFISSID2, PASSWORD2);
    
  if (SPIFFS.exists(WIFI_FILE)) {
    
    if (!loadWifiSettings()) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to load wifi config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tWifi config loaded");
      #endif
    }
  }
  else
    if (!setWifiSettings(WIFISSID,PASSWORD)) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save wifi config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tWifi config saved");
      #endif
    }

  if (SPIFFS.exists(THING_FILE)) {
    
    if (!loadThingSettings()) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to load Thingspeak config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tThingspeak config loaded");
      #endif
    }
  }
  else {
      #ifdef DEBUG
        Serial.println("[INFO]\tNo Thingspeak config available");
      #endif
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// React to Serial Input 
void read_serial(char *buffer) 
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buffer);
  
  if (!json.success()) {
    Serial.println("String invalid");
    return;
  }

  // new command
  const char* command = json["command"];

  // ADD WIFI SETTINGS
  if (strcmp(command, "addWIFI")==0) {
    //const char* ssid = json["data"][0];
    //const char* pass = json["data"][1];

    if (!addWifiSettings(json["data"][0], json["data"][1])) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save wifi config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tWifi config saved");
     #endif
    }
  }

  // SET WIFI SETTINGS
  else if (strcmp(command, "setWIFI")==0) {
    //const char* ssid = json["data"][0];
    //const char* pass = json["data"][1];
    
    if (!setWifiSettings(json["data"][0], json["data"][1])) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save wifi config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tWifi config saved");
      #endif
    } 
    
  }

  // GET CURRENT WIFI SSID
  else if (strcmp(command, "getSSID")==0) {
    Serial.println(WiFi.SSID());
  }

  // SET THINGSPEAK KEY
  else if (strcmp(command, "setTS")==0) {

    //const char* key = json["data"][0];
    
    if (!setThingSettings(json["data"][0])) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save Thingspeak config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tThingspeak config saved");
      #endif
    }
    
  }

  // GET THINGSPEAK KEY
  else if (strcmp(command, "getTS")==0) {
    Serial.println(THINGSPEAK_KEY);
  }

  // RESTART SYSTEM
  else if (strcmp(command, "restart")==0) {
    ESP.restart();
  }

  // GET FIRMWAREVERSION
  else if (strcmp(command, "getVersion")==0) {
    Serial.println(FIRMWAREVERSION);
  }

  // GET HELP
  else if (strcmp(command, "help")==0) {
    Serial.println();
    Serial.println("Syntax: {\"command\":\"xxx\",\"data\":[\"xxx\",\"xxx\"]}");
    Serial.println("Possible commands");
    Serial.println("restart    -> Restart ESP");
    Serial.println("getVersion -> Show Firmware Version Number");
    Serial.println("getSSID    -> Show current SSID");
    Serial.println("setWIFI    -> Reset wifi.json and add new SSID");
    Serial.println("           -> expected data SSID and PASSWORD");
    Serial.println("addWIFI    -> Add new SSID to wifi.json");
    Serial.println("           -> expected data SSID and PASSWORD");
    Serial.println("setTS      -> Add THINGSPEAK KEY");
    Serial.println("           -> expected data KEY");
    Serial.println("getTS      -> Show THINGSPEAK KEY");
    Serial.println();
  }

  else Serial.println("Unkwown command");     // Befehl nicht erkannt
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Put together Serial Input 
int readline(int readch, char *buffer, int len) {
  
  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}

