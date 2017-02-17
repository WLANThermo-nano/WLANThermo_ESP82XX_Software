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
// Load xxx.json
bool loadfile(const char* filename, File& configFile) {
  
  configFile = SPIFFS.open(filename, "r");
  if (!configFile) {
    #ifdef DEBUG
    Serial.print("[INFO]\tFailed to open: ");
    Serial.println(filename);
    #endif
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    #ifdef DEBUG
    Serial.print("[INFO]\tFile size is too large: ");
    Serial.println(filename);
    #endif
    return false;
  }
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Save xxx.json
bool savefile(const char* filename, File& configFile) {
  
  configFile = SPIFFS.open(filename, "w");
  
  if (!configFile) {
    #ifdef DEBUG
    Serial.print("Failed to open file for writing: ");
    Serial.println(filename);
    #endif
    return false;
  }  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check JSON OBJECT
bool checkjsonobject(JsonObject& json, const char* filename) {
  
  if (!json.success()) {
    #ifdef DEBUG
    Serial.print("Failed to parse: ");
    Serial.println(filename);
    #endif
    return false;
  }
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check JSON ARRAY
bool checkjsonarray(JsonArray& json, const char* filename) {
  
  if (!json.success()) {
    #ifdef DEBUG
    Serial.print("Failed to parse: ");
    Serial.println(filename);
    #endif
    return false;
  }
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load xxx.json at system start
bool loadconfig(byte count) {

  DynamicJsonBuffer jsonBuffer;
  File configFile;

  switch (count) {
    
    case 0:     // CHANNEL
    {
      //if (!loadfile(CHANNEL_FILE,configFile)) return false;
      //std::unique_ptr<char[]> buf(new char[configFile.size()]);
      //configFile.readBytes(buf.get(), configFile.size());
      //configFile.close();

      std::unique_ptr<char[]> buf(new char[500]);
      readEE(buf.get(),500, 400);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjsonobject(json,CHANNEL_FILE)) return false;
      if (json["VERSION"] != CHANNELJSONVERSION) return false;
  
      const char* author = json["AUTHOR"];
      temp_unit = json["temp_unit"].asString();

      for (int i=0; i < CHANNELS; i++){
          ch[i].typ = json["ttyp"][i];            
          ch[i].min = json["tmin"][i];            
          ch[i].max = json["tmax"][i];            
          ch[i].soll = json["tsoll"][i];          
          ch[i].alarm = json["talarm"][i];        
          ch[i].color = json["tcolor"][i].asString();        
      }
    }
    break;
    
    case 1:     // WIFI
    {
      std::unique_ptr<char[]> buf(new char[300]);
      readEE(buf.get(),300, 0);

      JsonArray& _wifi = jsonBuffer.parseArray(buf.get());
      if (!checkjsonarray(_wifi,WIFI_FILE)) return false;
      
      // Wie viele WLAN Schlüssel sind vorhanden
      for (JsonArray::iterator it=_wifi.begin(); it!=_wifi.end(); ++it) {  
        wifissid[lenwifi] = _wifi[lenwifi]["SSID"].asString();
        wifipass[lenwifi] = _wifi[lenwifi]["PASS"].asString();  
        lenwifi++;
      }
    }
    break;
  
    case 2:     // THINGSPEAK
    { 
      std::unique_ptr<char[]> buf(new char[100]);
      readEE(buf.get(),100, 300);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjsonobject(json,THING_FILE)) return false;
      THINGSPEAK_KEY = json["KEY"].asString();
    }
    break;

    
    //case 3:     // PRESETS
    //break;
  
    default:
    return false;
  
  }

  /*
  #ifdef DEBUG
  Serial.print("[JSON GET]\t");
  json.printTo(Serial);
  Serial.println();
  #endif
  */
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set xxx.json
bool setconfig(byte count, const char* data1, const char* data2) {
  
  DynamicJsonBuffer jsonBuffer;
  File configFile;

  switch (count) {
    case 0:         // CHANNEL
    {
      JsonObject& json = jsonBuffer.createObject();
  
      json["AUTHOR"] = "s.ochs";
      json["VERSION"] = CHANNELJSONVERSION;
      json["temp_unit"] = temp_unit;

      JsonArray& _typ = json.createNestedArray("ttyp");
      JsonArray& _min = json.createNestedArray("tmin");
      JsonArray& _max = json.createNestedArray("tmax");
      JsonArray& _soll = json.createNestedArray("tsoll");
      JsonArray& _alarm = json.createNestedArray("talarm");
      JsonArray& _color = json.createNestedArray("tcolor");
  
      for (int i=0; i < CHANNELS; i++){
        _typ.add(0);
    
        if (temp_unit == "F") {
          _min.add(68.0,1);
          _max.add(86.0,1);
          _soll.add(75.0,1);
        } else {
          _min.add(20.0,1);
          _max.add(30.0,1);
          _soll.add(25.0,1); 
        }
        _alarm.add(false); 
        _color.add(colors[i]);
      }

      //if (!savefile(CHANNEL_FILE, configFile)) return false;
      //json.printTo(configFile);
      //configFile.close();
      size_t size = json.measureLength() + 1;
      clearEE(500,400);  // Bereich reinigen
      static char buffer[500];
      json.printTo(buffer, size);
      writeEE(buffer, size, 400);
    }
    break;

    case 1:        // WIFI
    {
      JsonArray& json = jsonBuffer.createArray();
      clearEE(300,0);  // Bereich reinigen
      static char buffer[3];
      json.printTo(buffer, 3);
      writeEE(buffer, 3, 0);
    }
    break;
    
    case 2:         //THING
    {
      JsonObject& json = jsonBuffer.createObject();
      THINGSPEAK_KEY = data1;
      json["KEY"] = THINGSPEAK_KEY;
      
      size_t size = json.measureLength() + 1;
      if (size > 100) {
        #ifdef DEBUG
        Serial.println("[INFO]\tZu viele THINGSPEAK Daten!");
        #endif
        return false;
      } else {
        clearEE(100,300);  // Bereich reinigen
        static char buffer[100];
        json.printTo(buffer, size);
        writeEE(buffer, size, 300);
      }
    }
    break;

    case 3:         //PRESETS
    {
      
    }
    break;

    default:
    return false;
  
  }
      
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Modify xxx.json
bool modifyconfig(byte count, const char* data1, const char* data2) {
  
  DynamicJsonBuffer jsonBuffer;
  File configFile;

  switch (count) {
    case 0:           // CHANNEL
    {
      // Alte Daten auslesen
      //if (!loadfile(CHANNEL_FILE,configFile)) return false;
      //std::unique_ptr<char[]> buf(new char[configFile.size()]);
      //configFile.readBytes(buf.get(), configFile.size());
      //configFile.close();
      
      std::unique_ptr<char[]> buf(new char[500]);
      readEE(buf.get(),500, 400);
      
      JsonObject& alt = jsonBuffer.parseObject(buf.get());
      if (!checkjsonobject(alt,CHANNEL_FILE)) return false;
      
      // Neue Daten erzeugen
      JsonObject& json = jsonBuffer.createObject();

      json["AUTHOR"] = alt["AUTHOR"];
      json["VERSION"] = CHANNELJSONVERSION;
      json["temp_unit"] = temp_unit;

      JsonArray& _typ = json.createNestedArray("ttyp");
      JsonArray& _min = json.createNestedArray("tmin");
      JsonArray& _max = json.createNestedArray("tmax");
      JsonArray& _soll = json.createNestedArray("tsoll");
      JsonArray& _alarm = json.createNestedArray("talarm");
      JsonArray& _color = json.createNestedArray("tcolor");
    
      for (int i=0; i < CHANNELS; i++){
        _typ.add(ch[i].typ); 
        _min.add(ch[i].min,1);
        _max.add(ch[i].max,1);
        _soll.add(ch[i].soll,1);
        _alarm.add(ch[i].alarm); 
        _color.add(ch[i].color);
      }

      // Speichern
      //if (!savefile(CHANNEL_FILE, configFile)) return false;
      //json.printTo(configFile);
      //configFile.close();
      size_t size = json.measureLength() + 1;
      clearEE(500,400);  // Bereich reinigen
      static char buffer[500];
      json.printTo(buffer, size);
      writeEE(buffer, size, 400);

      //Serial.print("[JSON SET]\t");
      //json.printTo(Serial);
      //Serial.println();
      
    }
    break;

    case 1:         // WIFI
    {
      // Alte Daten auslesen
      std::unique_ptr<char[]> buf(new char[300]);
      readEE(buf.get(), 300, 0);

      JsonArray& json = jsonBuffer.parseArray(buf.get());
      if (!checkjsonarray(json,WIFI_FILE)) {
        setconfig(eWIFI,"","");
        return false;
      }

      // Neue Daten eintragen
      JsonObject& _wifi = json.createNestedObject();

      _wifi["SSID"] = data1;
      _wifi["PASS"] = data2;

      // Speichern
      size_t size = json.measureLength() + 1;
      
      if (size > 300) {
        #ifdef DEBUG
        Serial.println("[INFO]\tZu viele WLAN Daten!");
        #endif
        return false;
      } else {
        static char buffer[300];
        json.printTo(buffer, size);
        writeEE(buffer, size, 0); 
      } 
    }
    break;
    
    case 2:         //THING
    break;

    default:
    return false;
  }
  
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
  Dir dir = SPIFFS.openDir("/");  // Alternativ: Dir dir = SPIFFS.openDir("/data");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  */
  #ifdef DEBUG
    Serial.print("[INFO]\tInitalize SPIFFS at Sector: 0x");
    Serial.print((((uint32_t)&_SPIFFS_start - 0x40200000) / SPI_FLASH_SEC_SIZE), HEX);
    Serial.print(" (");
    Serial.print(((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start)/1024, DEC);
    Serial.println("K)");
    // 0x40200000 ist der Speicherort des SPI FLASH in der Memory Map
  #endif

  // CHANNEL
  if (!loadconfig(eCHANNEL)) {
    #ifdef DEBUG
      Serial.println("[INFO]\tFailed to load channel config");
    #endif
    setconfig(eCHANNEL,"","");  // Speicherplatz vorbereiten
    ESP.restart();
  } else {
    #ifdef DEBUG
      Serial.println("[INFO]\tChannel config loaded");
    #endif
  }

  /*
  // CHANNEL
  if (SPIFFS.exists(CHANNEL_FILE)) {
    
    if (!loadconfig(eCHANNEL)) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to load channel config");
      #endif

      // Falsche Version ueberschreiben
      if (!setconfig(eCHANNEL,"","")) {
        #ifdef DEBUG
          Serial.println("[INFO]\tFailed to save channel config");
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
    if (!setconfig(eCHANNEL,"","")) {
      #ifdef DEBUG
        Serial.println("[INFO]\tFailed to save channel config");
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("[INFO]\tChannel config saved");
      #endif
      ESP.restart();
    }
    */


  // WIFI
  if (!loadconfig(eWIFI)) {
    #ifdef DEBUG
      Serial.println("[INFO]\tFailed to load wifi config");
    #endif
    setconfig(eWIFI,"","");  // Speicherplatz vorbereiten
  } else {
    #ifdef DEBUG
      Serial.println("[INFO]\tWifi config loaded");
    #endif
  }


  // THINGSPEAK
  if (!loadconfig(eTHING)) {
    #ifdef DEBUG
      Serial.println("[INFO]\tFailed to load Thingspeak config");
    #endif
  } else {
    #ifdef DEBUG
      Serial.println("[INFO]\tThingspeak config loaded");
    #endif
  }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// React to Serial Input 
void read_serial(char *buffer) {

  // GET HELP
  if (strcmp(buffer, "help")==0) {
    Serial.println();
    Serial.println("Syntax: {\"command\":\"xxx\",\"data\":[\"xxx\",\"xxx\"]}");
    Serial.println("Possible commands");
    Serial.println("restart    -> Restart ESP");
    Serial.println("getVersion -> Show Firmware Version Number");
    Serial.println("getSSID    -> Show current SSID");
    Serial.println("setWIFI    -> Reset wifi.json");
    Serial.println("           -> expected no data");
    Serial.println("addWIFI    -> Add new SSID to wifi.json");
    Serial.println("           -> expected data SSID and PASSWORD");
    Serial.println("setTS      -> Add THINGSPEAK KEY");
    Serial.println("           -> expected data KEY");
    Serial.println("getTS      -> Show THINGSPEAK KEY");
    Serial.println();
    return;
  }
  else if (strcmp(buffer, "data")==0) {
    static char sendbuffer[1000];
    buildDatajson(sendbuffer, 1000);
    Serial.println(sendbuffer);
    return;
  }
  else if (strcmp(buffer, "settings")==0) {
    static char sendbuffer[200];
    buildSettingjson(sendbuffer, 200);
    Serial.println(sendbuffer);
    return;
  }

  Serial.print("You entered: >");
  Serial.print(buffer);
  Serial.println("<");

  // Wenn nicht help dann json-Befehl auslesen
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

    if (!modifyconfig(eWIFI,json["data"][0], json["data"][1])) {
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
        
    if (!setconfig(eWIFI,"","")) {  
    #ifdef DEBUG
      Serial.println("[INFO]\tReset wifi config");
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
    
    if (!setconfig(eTHING,json["data"][0],"")) {
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

  // LET ESP SLEEP
  else if (strcmp(command, "sleep")==0) {
    display.displayOff();
    ESP.deepSleep(0);
    delay(100); // notwendig um Prozesse zu beenden
  }

  // GET FIRMWAREVERSION
  else if (strcmp(command, "getVersion")==0) {
    Serial.println(FIRMWAREVERSION);
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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initalize EEPROM
void setEE() {
  
  // EEPROM Sector: 0xFB, ab Sector 0xFC liegen System Parameter
  
  #ifdef DEBUG
  Serial.print("[INFO]\tInitalize EEPROM at Sector: 0x"); // letzter Sector von APP2
  Serial.print((((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE),HEX);
  Serial.print(" (");
  Serial.print(EEPROM_SIZE, DEC);  // ESP.getFreeSketchSpace()
  Serial.println("B)");
  #endif
  
  EEPROM.begin(EEPROM_SIZE);
}

void writeEE(const char* json, int len, int startP) {
  
  Serial.print("[INFO]\tWriting to EE: (");
  Serial.print(len);
  Serial.print(") ");
  Serial.println(json);
  for (int i = startP; i < (startP+len); ++i)
    {
    EEPROM.write(i, json[i-startP]);
    } 
  EEPROM.commit();
}


void readEE(char *buffer, int len, int startP) {
  
  for (int i = startP; i < (startP+len); ++i) {
    buffer[i-startP] = char(EEPROM.read(i));
  }
  Serial.print("[INFO]\tReading from EE: ");
  Serial.println(buffer);
}

void clearEE(int len, int startP) {  
  
  Serial.print("[INFO]\tClear EEPROM from: ");
  Serial.print(startP);
  Serial.print(" to: ");
  Serial.println(startP+len-1);
  for (int i = startP; i < (startP+len); ++i)
    {
    EEPROM.write(i, 0);
    } 
  EEPROM.commit();
}



