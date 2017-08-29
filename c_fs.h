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

 // HELP: https://github.com/bblanchon/ArduinoJson

#define CHANNEL_FILE  "/channel.json"
#define WIFI_FILE     "/wifi.json"
#define THING_FILE    "/thing.json"
#define PIT_FILE      "/pit.json"
#define SYSTEM_FILE   "/system.json"

/*
//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load xxx.json
bool loadfile(const char* filename, File& configFile) {
  
  configFile = SPIFFS.open(filename, "r");
  if (!configFile) {
    
    DPRINTP("[INFO]\tFailed to open: ");
    DPRINTLN(filename);
    
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    
    DPRINTP("[INFO]\tFile size is too large: ");
    DPRINTLN(filename);
    
    return false;
  }
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Save xxx.json
bool savefile(const char* filename, File& configFile) {
  
  configFile = SPIFFS.open(filename, "w");
  
  if (!configFile) {
    
    DPRINTP("[INFO]\tFailed to open file for writing: ");
    DPRINTLN(filename);
    
    return false;
  }  
  return true;
}
*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check JSON
bool checkjson(JsonVariant json, const char* filename) {
  
  if (!json.success()) {
    
    DPRINTP("[INFO]\tFailed to parse: ");
    DPRINTLN(filename);
    
    return false;
  }
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load xxx.json at system start
bool loadconfig(byte count) {

  const size_t bufferSize = 6*JSON_ARRAY_SIZE(CHANNELS) + JSON_OBJECT_SIZE(9) + 320;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  //File configFile;

  switch (count) {
    
    case 0:     // CHANNEL
    {

      std::unique_ptr<char[]> buf(new char[EECHANNEL]);
      readEE(buf.get(),EECHANNEL, EECHANNELBEGIN);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,CHANNEL_FILE)) return false;
      
      if (json.containsKey("temp_unit"))  temp_unit = json["temp_unit"].asString();
      else return false;
      
      for (int i=0; i < CHANNELS; i++){
          ch[i].name = json["tname"][i].asString();
          ch[i].typ = json["ttyp"][i];            
          ch[i].min = json["tmin"][i];            
          ch[i].max = json["tmax"][i];                     
          ch[i].alarm = json["talarm"][i];        
          ch[i].color = json["tcolor"][i].asString();        
      }
    }
    break;
    
    case 1:     // WIFI
    {
      std::unique_ptr<char[]> buf(new char[EEWIFI]);
      readEE(buf.get(),EEWIFI, EEWIFIBEGIN);

      JsonArray& _wifi = jsonBuffer.parseArray(buf.get());
      if (!checkjson(_wifi,WIFI_FILE)) return false;
      
      // Wie viele WLAN Schlüssel sind vorhanden
      for (JsonArray::iterator it=_wifi.begin(); it!=_wifi.end(); ++it) {  
        wifissid[lenwifi] = _wifi[lenwifi]["SSID"].asString();
        wifipass[lenwifi] = _wifi[lenwifi]["PASS"].asString();  
        lenwifi++;
      }
    }
    break;
  
    case 2:     // IOT
    { 
      std::unique_ptr<char[]> buf(new char[EETHING]);
      readEE(buf.get(),EETHING, EETHINGBEGIN);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,THING_FILE)) return false;
      
      if (json.containsKey("TSwrite"))  iot.TS_writeKey = json["TSwrite"].asString();
      else return false;
      if (json.containsKey("TShttp"))   iot.TS_httpKey = json["TShttp"].asString();
      else return false;
      if (json.containsKey("TSchID"))   iot.TS_chID = json["TSchID"].asString();
      else return false;
      if (json.containsKey("TS8"))      iot.TS_show8 = json["TS8"];
      else return false;
      if (json.containsKey("TSuser"))   iot.TS_userKey = json["TSuser"].asString();
      else return false;
      if (json.containsKey("TSint"))    iot.TS_int = json["TSint"];
      else return false;
      if (json.containsKey("TSon"))     iot.TS_on = json["TSon"];
      else return false;
      if (json.containsKey("PMQhost"))  iot.P_MQTT_HOST = json["PMQhost"].asString();
      else return false;
      if (json.containsKey("PMQport"))  iot.P_MQTT_PORT = json["PMQport"];
      else return false;
      if (json.containsKey("PMQuser"))  iot.P_MQTT_USER = json["PMQuser"].asString();
      else return false;
      if (json.containsKey("PMQpass"))  iot.P_MQTT_PASS = json["PMQpass"].asString();
      else return false;
      if (json.containsKey("PMQqos"))   iot.P_MQTT_QoS = json["PMQqos"];
      else return false;
      if (json.containsKey("PMQon"))    iot.P_MQTT_on = json["PMQon"];
      else return false;
      if (json.containsKey("PMQint"))   iot.P_MQTT_int = json["PMQint"];
      else return false;
      if (json.containsKey("TGon"))     iot.TG_on = json["TGon"];
      else return false;
      if (json.containsKey("TGtoken"))  iot.TG_token = json["TGtoken"].asString();
      else return false;
      if (json.containsKey("TGid"))     iot.TG_id = json["TGid"].asString(); 
      else return false;
      if (json.containsKey("CLon"))     iot.CL_on = json["CLon"];
      else return false;
      if (json.containsKey("CLtoken"))  iot.CL_token = json["CLtoken"].asString();
      else return false;
      if (json.containsKey("CLint"))    iot.CL_int = json["CLint"];
      else return false;
    }
    break;

    case 3:     // PITMASTER
    {
      std::unique_ptr<char[]> buf(new char[EEPITMASTER]);
      readEE(buf.get(),EEPITMASTER, EEPITMASTERBEGIN);

      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,PIT_FILE)) return false;

      JsonObject& _master = json["pm"];

      if (_master.containsKey("ch"))  pitmaster.channel   = _master["ch"]; 
      else return false;
      pitmaster.pid       = _master["pid"];
      pitmaster.set       = _master["set"];
      pitmaster.active    = _master["act"];
      pitmaster.resume    = _master["res"];
  
      JsonArray& _pid = json["pid"];

      pidsize = 0;
      
      // Wie viele Pitmaster sind vorhanden
      for (JsonArray::iterator it=_pid.begin(); it!=_pid.end(); ++it) {  
        pid[pidsize].name = _pid[pidsize]["name"].asString();
        pid[pidsize].id = _pid[pidsize]["id"];
        pid[pidsize].aktor = _pid[pidsize]["aktor"];  
        pid[pidsize].Kp = _pid[pidsize]["Kp"];  
        pid[pidsize].Ki = _pid[pidsize]["Ki"];    
        pid[pidsize].Kd = _pid[pidsize]["Kd"];                     
        pid[pidsize].Kp_a = _pid[pidsize]["Kp_a"];                   
        pid[pidsize].Ki_a = _pid[pidsize]["Ki_a"];                   
        pid[pidsize].Kd_a = _pid[pidsize]["Kd_a"];                   
        pid[pidsize].Ki_min = _pid[pidsize]["Ki_min"];                   
        pid[pidsize].Ki_max = _pid[pidsize]["Ki_max"];                  
        pid[pidsize].pswitch = _pid[pidsize]["switch"];               
        pid[pidsize].reversal = _pid[pidsize]["rev"];                
        pid[pidsize].DCmin    = _pid[pidsize]["DCmin"];              
        pid[pidsize].DCmax    = _pid[pidsize]["DCmax"];              
        pid[pidsize].SVmin    = _pid[pidsize]["SVmin"];             
        pid[pidsize].SVmax    = _pid[pidsize]["SVmax"];   
        pid[pidsize].esum = 0;             
        pid[pidsize].elast = 0;       
        pidsize++;
      }
    }
    if (pidsize < 3) return 0;   // Alte Versionen abfangen
    break;

    case 4:     // SYSTEM
    {
      std::unique_ptr<char[]> buf(new char[EESYSTEM]);
      readEE(buf.get(),EESYSTEM, EESYSTEMBEGIN);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,SYSTEM_FILE)) return false;
  
      if (json.containsKey("host"))     sys.host = json["host"].asString();
      else return false;
      if (json.containsKey("hwalarm"))  sys.hwalarm = json["hwalarm"];
      else return false;
      if (json.containsKey("ap"))       sys.apname = json["ap"].asString();
      else return false;
      if (json.containsKey("lang"))     sys.language = json["lang"].asString();
      else return false;
      if (json.containsKey("batmax"))   battery.max = json["batmax"];
      else return false;
      if (json.containsKey("batmin"))   battery.min = json["batmin"];
      else return false;
      if (json.containsKey("logsec")) {
        int sector = json["logsec"];
        if (sector > log_sector) log_sector = sector;
        // oberes limit wird spaeter abgefragt
      }
      else return false;
      if (json.containsKey("fast"))     sys.fastmode = json["fast"];
      else return false;
      if (json.containsKey("hwversion")) sys.hwversion = json["hwversion"];
      else return false;
      if (json.containsKey("update"))   sys.update = json["update"];
      else return false;
      if (json.containsKey("autoupd"))  sys.autoupdate = json["autoupd"];
      else return false;
      if (json.containsKey("getupd"))   sys.getupdate = json["getupd"].asString();
      else return false;
      if (json.containsKey("god"))      sys.god = json["god"];
      else return false;
      if (json.containsKey("pitsup"))      sys.pitsupply = json["pitsup"];
      //else return false;
      
    }
    break;
    
    //case 5:     // PRESETS
    //break;
  
    default:
    return false;
  
  }

  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set xxx.json
bool setconfig(byte count, const char* data[2]) {
  
  DynamicJsonBuffer jsonBuffer;
  //File configFile;

  switch (count) {
    case 0:         // CHANNEL
    {
      JsonObject& json = jsonBuffer.createObject();

      json["temp_unit"] = temp_unit;

      JsonArray& _name = json.createNestedArray("tname");
      JsonArray& _typ = json.createNestedArray("ttyp");
      JsonArray& _min = json.createNestedArray("tmin");
      JsonArray& _max = json.createNestedArray("tmax");
      JsonArray& _alarm = json.createNestedArray("talarm");
      JsonArray& _color = json.createNestedArray("tcolor");
    
      for (int i=0; i < CHANNELS; i++){
        _name.add(ch[i].name);
        _typ.add(ch[i].typ); 
        _min.add(ch[i].min,1);
        _max.add(ch[i].max,1);
        _alarm.add(ch[i].alarm); 
        _color.add(ch[i].color);
      }

      //if (!savefile(CHANNEL_FILE, configFile)) return false;
      //json.printTo(configFile);
      //configFile.close();
      size_t size = json.measureLength() + 1;
      clearEE(EECHANNEL,EECHANNELBEGIN);  // Bereich reinigen
      static char buffer[EECHANNEL];
      json.printTo(buffer, size);
      writeEE(buffer, size, EECHANNELBEGIN);
    }
    break;

    case 1:        // WIFI
    {
      JsonArray& json = jsonBuffer.createArray();
      clearEE(EEWIFI,EEWIFIBEGIN);  // Bereich reinigen
      static char buffer[3];
      json.printTo(buffer, 3);
      writeEE(buffer, 3, EEWIFIBEGIN);
    }
    break;
    
    case 2:         //IOT
    {
      JsonObject& json = jsonBuffer.createObject();
      
      json["TSwrite"] = iot.TS_writeKey;
      json["TShttp"]  = iot.TS_httpKey;
      json["TSuser"]  = iot.TS_userKey;
      json["TSchID"]  = iot.TS_chID;
      json["TS8"]     = iot.TS_show8;
      json["TSint"]   = iot.TS_int;
      json["TSon"]    = iot.TS_on;
      json["PMQhost"]  = iot.P_MQTT_HOST;
      json["PMQport"]  = iot.P_MQTT_PORT;
      json["PMQuser"]  = iot.P_MQTT_USER;
      json["PMQpass"]  = iot.P_MQTT_PASS;
      json["PMQqos"]   = iot.P_MQTT_QoS;
      json["PMQon"]   = iot.P_MQTT_on;
      json["PMQint"]   = iot.P_MQTT_int;
      json["TGon"]    = iot.TG_on;
      json["TGtoken"] = iot.TG_token;
      json["TGid"]    = iot.TG_id;
      json["CLon"]    = iot.CL_on;
      json["CLtoken"] = iot.CL_token;
      json["CLint"]   = iot.CL_int;
      
      size_t size = json.measureLength() + 1;
      if (size > EETHING) {
        DPRINTPLN("[INFO]\tZu viele BOT Daten!");
        DPRINTLN("[INFO]\tFailed to save Thingspeak config");
        return false;
      } else {
        clearEE(EETHING,EETHINGBEGIN);  // Bereich reinigen
        static char buffer[EETHING];
        json.printTo(buffer, size);
        writeEE(buffer, size, EETHINGBEGIN);
      }
    }
    break;

    case 3:        // PITMASTER
    {
      JsonObject& json = jsonBuffer.createObject();
      JsonObject& _master = json.createNestedObject("pm");
      
      _master["ch"]     = pitmaster.channel;
      _master["pid"]    = pitmaster.pid;
      _master["set"]    = pitmaster.set;
      _master["act"]    = pitmaster.active;
      _master["res"]    = pitmaster.resume;
  
      JsonArray& _pit = json.createNestedArray("pid");
  
      for (int i = 0; i < pidsize; i++) {
        JsonObject& _pid = _pit.createNestedObject();
        _pid["name"]     = pid[i].name;
        _pid["id"]       = pid[i].id;
        _pid["aktor"]    = pid[i].aktor;
        _pid["Kp"]       = pid[i].Kp;  
        _pid["Ki"]       = pid[i].Ki;    
        _pid["Kd"]       = pid[i].Kd;                   
        _pid["Kp_a"]     = pid[i].Kp_a;               
        _pid["Ki_a"]     = pid[i].Ki_a;                  
        _pid["Kd_a"]     = pid[i].Kd_a;             
        _pid["Ki_min"]   = pid[i].Ki_min;             
        _pid["Ki_max"]   = pid[i].Ki_max;             
        _pid["switch"]   = pid[i].pswitch;           
        _pid["rev"]      = pid[i].reversal;                
        _pid["DCmin"]    = pid[i].DCmin;             
        _pid["DCmax"]    = pid[i].DCmax;             
        _pid["SVmin"]    = pid[i].SVmin;             
        _pid["SVmax"]    = pid[i].SVmax;
      }
       
      size_t size = json.measureLength() + 1;
      if (size > EEPITMASTER) {
        DPRINTPLN("[INFO]\tZu viele PITMASTER Daten!");
        DPRINTPLN("[INFO]\tFailed to save Pitmaster config");
        return false;
      } else {
        clearEE(EEPITMASTER,EEPITMASTERBEGIN);  // Bereich reinigen
        static char buffer[EEPITMASTER];
        json.printTo(buffer, size);
        writeEE(buffer, size, EEPITMASTERBEGIN);
      }
    }
    break;

    case 4:         // SYSTEM
    {
      JsonObject& json = jsonBuffer.createObject();
      
      json["host"] =        sys.host;
      json["hwalarm"] =     sys.hwalarm;
      json["ap"] =          sys.apname;
      json["lang"] =        sys.language;
      json["fast"] =        sys.fastmode;
      json["hwversion"] =   sys.hwversion;
      json["update"] =      sys.update;
      json["getupd"] =      sys.getupdate;
      json["autoupd"] =     sys.autoupdate;
      json["batmax"] =      battery.max;
      json["batmin"] =      battery.min;
      json["logsec"] =      log_sector;
      json["god"] =         sys.god;
      json["pitsup"] =      sys.pitsupply;
    
      size_t size = json.measureLength() + 1;
      clearEE(EESYSTEM,EESYSTEMBEGIN);  // Bereich reinigen
      static char buffer[EESYSTEM];
      json.printTo(buffer, size);
      writeEE(buffer, size, EESYSTEMBEGIN);
    }
    break;

    case 5:         //PRESETS
    {
      
    }
    break;

    default:
    return false;
  
  }

  DPRINTPLN("[INFO]\tSaved!");
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Modify xxx.json
bool modifyconfig(byte count, const char* data[12]) {
  
  DynamicJsonBuffer jsonBuffer;

  switch (count) {
    case 0:           // CHANNEL
    // nicht notwendig, kann über setconfig beschrieben werden
    break;

    case 1:         // WIFI
    {
      // Alte Daten auslesen
      std::unique_ptr<char[]> buf(new char[EEWIFI]);
      readEE(buf.get(), EEWIFI, EEWIFIBEGIN);

      JsonArray& json = jsonBuffer.parseArray(buf.get());
      if (!checkjson(json,WIFI_FILE)) {
        setconfig(eWIFI,{});
        return false;
      }

      // Neue Daten eintragen
      JsonObject& _wifi = json.createNestedObject();

      _wifi["SSID"] = data[0];
      _wifi["PASS"] = data[1];
      
      // Speichern
      size_t size = json.measureLength() + 1;
      
      if (size > EEWIFI) {
        DPRINTPLN("[INFO]\tZu viele WIFI Daten!");
        return false;
      } else {
        clearEE(EEWIFI,EEWIFIBEGIN);  // Bereich reinigen
        static char buffer[EEWIFI];
        json.printTo(buffer, size);
        writeEE(buffer, size, EEWIFIBEGIN); 
      } 
    }
    break;
    
    case 2:         //THING
    // nicht notwendig, kann über setconfig beschrieben werden
    break;

    case 3:         // PITMASTER
    // nicht notwendig, kann über setconfig beschrieben werden
    break;

    case 4:           // SYSTEM
    // nicht notwendig, kann über setconfig beschrieben werden
    break;

    default:
    return false;
  }
  
  return true;
}

void serialNote(const char * data, bool art) {

  if (art) DPRINTP("[INFO]\tLoaded ");
  else     DPRINTP("[INFO]\tFailed to load ");
  DPRINTLN(data);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize FileSystem
void start_fs() {
  
  if (!SPIFFS.begin()) {
    DPRINTPLN("[INFO]\tFailed to mount file system");
    return;
  }

  DPRINTP("[INFO]\tInitalize SPIFFS at Sector: 0x");
  DPRINT((((uint32_t)&_SPIFFS_start - 0x40200000) / SPI_FLASH_SEC_SIZE), HEX);
  DPRINTP(" (");
  DPRINT(((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start)/1024, DEC);
  DPRINTPLN("K)");
  // 0x40200000 ist der Speicherort des SPI FLASH in der Memory Map

  String fileName;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    DPRINTP("[INFO]\tFS File: ");
    //fileName = dir.fileName();
    //size_t fileSize = dir.fileSize();
    //DPRINTF("[INFO]\tFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    DPRINT(dir.fileName());
    File f = dir.openFile("r");
    DPRINTP("\tSize: ");
    DPRINTLN(formatBytes(f.size()));
  }

  FSInfo fs_info;
  SPIFFS.info(fs_info);
  DPRINTP("[INFO]\tTotalBytes: ");
  DPRINT(formatBytes(fs_info.totalBytes));
  DPRINTP("\tUsedBytes: ");
  DPRINTLN(formatBytes(fs_info.usedBytes));
  //Serial.println(fs_info.blockSize);
  //Serial.println(fs_info.pageSize);

  //u32_t total, used;
  //int res = SPIFFS_info(&fs, &total, &used);
  
  // CHANNEL
  if (!loadconfig(eCHANNEL)) {
    serialNote(CHANNEL_FILE,0);
    set_channels(1);
    setconfig(eCHANNEL,{});  // Speicherplatz vorbereiten
    ESP.restart();
  } else serialNote(CHANNEL_FILE,1);


  // WIFI
  if (!loadconfig(eWIFI)) {
    serialNote(WIFI_FILE,0);
    setconfig(eWIFI,{});  // Speicherplatz vorbereiten
  } else serialNote(WIFI_FILE,1);


  // IOT
  if (!loadconfig(eTHING)) {
    serialNote(THING_FILE,0);
    set_iot(0);
    setconfig(eTHING,{});  // Speicherplatz vorbereiten
  } else serialNote(THING_FILE,1);


  // PITMASTER
  if (!loadconfig(ePIT)) {
    serialNote(PIT_FILE,0);
    set_pid();  // Default PID-Settings
    set_pitmaster(1);
    setconfig(ePIT,{});  // Reset pitmaster config
  } else serialNote(PIT_FILE,1);


  // SYSTEM
  if (!loadconfig(eSYSTEM)) {
    serialNote(SYSTEM_FILE,0);
    set_system();
    setconfig(eSYSTEM,{});  // Speicherplatz vorbereiten
    ESP.restart();
  } else serialNote(SYSTEM_FILE,1);

}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Test zum Speichern von Datalog

//unsigned char meinsatz[64] = "Ich nutze ab jetzt den Flash Speicher für meine Daten!\n";
//unsigned char meinflash[64];

void write_flash(uint32_t _sector) {

  noInterrupts();
  if(spi_flash_erase_sector(_sector) == SPI_FLASH_RESULT_OK) {  // ESP.flashEraseSector
    spi_flash_write(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) mylog, sizeof(mylog));  //ESP.flashWrite
    DPRINTP("[LOG]\tSpeicherung im Sector: ");
    DPRINTLN(_sector, HEX);
  } else DPRINTPLN("[INFO]\tFehler beim Speichern im Flash");
  interrupts(); 
}


void read_flash(uint32_t _sector) {

  noInterrupts();
  spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) archivlog, sizeof(archivlog));  //ESP.flashRead
  //spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) meinflash, sizeof(meinflash));
  interrupts();
}





