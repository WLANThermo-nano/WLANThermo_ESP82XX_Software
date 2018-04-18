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
#define LOG_FILE      "/log.txt"

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

/*
// Save Log
bool savelog() {
  
  if (millis() - lastUpdateDatalog > 60000) {

    File f = SPIFFS.open(LOG_FILE, "a");

    for (int i=0; i < CHANNELS; i++)  {
      if (ch[0].temp != INACTIVEVALUE) {
        f.print(String(ch[0].temp,1));    // 8 * 16 bit  // 8 * 2 byte
      } else {
        f.print("");
      }
      f.print("|");
    }
    //mylog[logc].timestamp = now();                                // 64 bit // 8 byte

    f.print((uint8_t) battery.percentage);           // 8  bit // 1 byte
    f.print("|");
    if (pitmaster1.active) {
      f.print((uint8_t) pitmaster1.value);            // 8  bit // 1 byte
      f.print("|");
      f.println((uint16_t) (pitmaster1.set * 10));           // 16 bit // 2 byte
    } else {
      f.println("");
    }
    
    f.close();
    Serial.println("Logeintrag");
    lastUpdateDatalog = millis();
  }
}

*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Check JSON
bool checkjson(JsonVariant json, const char* filename) {
  
  if (!json.success()) {
    
    IPRINTP("f: ");
    DPRINTLN(filename);
    
    return false;
  }
  
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Load xxx.json at system start
bool loadconfig(byte count, bool old) {

  const size_t bufferSize = 6*JSON_ARRAY_SIZE(CHANNELS) + JSON_OBJECT_SIZE(9) + 320;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  //File configFile;

  switch (count) {
    
    case 0:     // CHANNEL
    {

      std::unique_ptr<char[]> buf(new char[EECHANNEL]);
      if (!old) readEE(buf.get(),EECHANNEL, EECHANNELBEGIN);
      else readEE(buf.get(),EECHANNEL, 550);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,CHANNEL_FILE)) return false;
      
      if (json.containsKey("temp_unit"))  sys.unit = json["temp_unit"].asString();
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

      wifi.savedlen = 0;
      
      // Wie viele WLAN Schlüssel sind vorhanden
      for (JsonArray::iterator it=_wifi.begin(); it!=_wifi.end(); ++it) {  
        wifi.savedssid[wifi.savedlen] = _wifi[wifi.savedlen]["SSID"].asString();
        wifi.savedpass[wifi.savedlen] = _wifi[wifi.savedlen]["PASS"].asString();  
        wifi.savedlen++;
      }
    }
    break;
  
    case 2:     // IOT
    { 
      std::unique_ptr<char[]> buf(new char[EETHING]);
      if (!old) readEE(buf.get(),EETHING, EETHINGBEGIN);
      else readEE(buf.get(),EETHING, 1050);
      
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
      if (!old) readEE(buf.get(),EEPITMASTER, EEPITMASTERBEGIN);
      else readEE(buf.get(),EEPITMASTER, 1470);

      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,PIT_FILE)) return false;

      JsonArray& _master = json["pm"];

      byte pitsize = 0;

      for (JsonArray::iterator it=_master.begin(); it!=_master.end(); ++it) {
        pitMaster[pitsize].channel = _master[pitsize]["ch"];
        pitMaster[pitsize].pid = _master[pitsize]["pid"];
        pitMaster[pitsize].set = _master[pitsize]["set"];
        pitMaster[pitsize].active = _master[pitsize]["act"];
        pitMaster[pitsize].resume = _master[pitsize]["res"];

        if (pitMaster[pitsize].active == MANUAL && pitMaster[pitsize].resume) 
          pitMaster[pitsize].value = _master[pitsize]["val"];

        pitsize++;
      }
/*
      JsonObject& _master = json["pm"];

      Pitmaster pitmaster = pitmaster1;

      if (_master.containsKey("ch"))  pitmaster.channel   = _master["ch"]; 
      else return false;
      pitmaster.pid       = _master["pid"];
      pitmaster.set       = _master["set"];
      pitmaster.active    = _master["act"];
      pitmaster.resume    = _master["res"];

      if (pitmaster.active == MANUAL && pitmaster.resume) 
        if (_master.containsKey("val")) pitmaster.value = _master["val"];
  */
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
        //pid[pidsize].reversal = _pid[pidsize]["rev"];                
        pid[pidsize].DCmin    = _pid[pidsize]["DCmin"];              
        pid[pidsize].DCmax    = _pid[pidsize]["DCmax"];              
        //pid[pidsize].SVmin    = _pid[pidsize]["SVmin"];             
        //pid[pidsize].SVmax    = _pid[pidsize]["SVmax"];         
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
      if (json.containsKey("typk"))        sys.typk = json["typk"];
      //else return false;
      if (json.containsKey("batfull"))      battery.setreference = json["batfull"];
      //else return false;
      if (json.containsKey("pass"))      sys.www_password = json["pass"].asString();

      if (json.containsKey("damper"))      sys.damper = json["damper"];
      //if (json.containsKey("adp"))      sys.advanced = json["adp"];
      //if (json.containsKey("nobat"))    sys.nobattery = json["nobat"];
      
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

      json["temp_unit"] = sys.unit;

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
        IPRINTPLN("f:full");
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
      JsonArray& _master = json.createNestedArray("pm");
      
      for (int i = 0; i < PITMASTERSIZE; i++) {
        JsonObject& _ma = _master.createNestedObject();
      _ma["ch"]     = pitMaster[i].channel;
      _ma["pid"]    = pitMaster[i].pid;
      _ma["set"]    = double_with_n_digits(pitMaster[i].set,1);
      _ma["act"]    = pitMaster[i].active;
      _ma["res"]    = pitMaster[i].resume;
      _ma["val"]    = pitMaster[i].value;
      }
  
      JsonArray& _pit = json.createNestedArray("pid");
  
      for (int i = 0; i < pidsize; i++) {
        JsonObject& _pid = _pit.createNestedObject();
        _pid["name"]     = pid[i].name;
        _pid["id"]       = pid[i].id;
        _pid["aktor"]    = pid[i].aktor;
        _pid["Kp"]       = double_with_n_digits(pid[i].Kp,1);  
        _pid["Ki"]       = double_with_n_digits(pid[i].Ki,3);    
        _pid["Kd"]       = double_with_n_digits(pid[i].Kd,1);                   
        _pid["Kp_a"]     = double_with_n_digits(pid[i].Kp_a,1);               
        _pid["Ki_a"]     = double_with_n_digits(pid[i].Ki_a,3);                  
        _pid["Kd_a"]     = double_with_n_digits(pid[i].Kd_a,1);             
        _pid["Ki_min"]   = pid[i].Ki_min;             
        _pid["Ki_max"]   = pid[i].Ki_max;             
        _pid["switch"]   = pid[i].pswitch;                           
        _pid["DCmin"]    = double_with_n_digits(pid[i].DCmin,1);             
        _pid["DCmax"]    = double_with_n_digits(pid[i].DCmax,1);             
        //_pid["SVmin"]    = pid[i].SVmin;             
        //_pid["SVmax"]    = pid[i].SVmax;
      }
       
      size_t size = json.measureLength() + 1;
      if (size > EEPITMASTER) {
        IPRINTPLN("f:full");
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
      json["typk"] =        sys.typk;
      json["pitsup"] =      sys.pitsupply;
      json["batfull"] =     battery.setreference;
      json["pass"] =        sys.www_password;
      json["damper"] =      sys.damper;
      //json["adp"] =        sys.advanced;
      //json["nobat"] =       sys.nobattery;
    
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
  sys.sendSettingsflag = true;
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Modify xxx.json
bool modifyconfig(byte count, bool neu) {
  
  DynamicJsonBuffer jsonBuffer;

  switch (count) {
    case 0:           // CHANNEL
    // nicht notwendig, kann über setconfig beschrieben werden
    break;

    case 1:         // WIFI
    {
      
      JsonArray& json = jsonBuffer.createArray();

      if (neu) {
        question.typ = IPADRESSE;     // Notification IP Adresse
        drawQuestion(0);
        //if (wifi.savedlen > 0)        // schon Daten vorhanden
          wifi.savedlen++;  // neue Daten
      }
      
      Serial.println("Sort SSID: ");
      Serial.println(wifi.savedlen);
      
      JsonObject& _wifi = json.createNestedObject();
      _wifi["SSID"] = holdssid.ssid;
      _wifi["PASS"] = holdssid.pass;

      for (int i = 0; i < wifi.savedlen; ++i) {
        if ((wifi.savedssid[i] != holdssid.ssid) && (wifi.savedssid[i] != "")) {  // doppelt aussortieren
          JsonObject& _wifi = json.createNestedObject();
          _wifi["SSID"] = wifi.savedssid[i];
          _wifi["PASS"] = wifi.savedpass[i];
        }
      }

      // Speichern
      size_t size = json.measureLength() + 1;
      
      if (size > EEWIFI) {
        IPRINTPLN("f:full");
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

  if (art) {IPRINTP("l: ");}      // Load
  else     {IPRINTP("f: ");}      // Failed Load
  DPRINTLN(data);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize FileSystem
void start_fs() {
  
  if (!SPIFFS.begin()) {
    DPRINTPLN("f:FS");
    return;
  }

  IPRINTP("SPIFFS: 0x");
  DPRINT((((uint32_t)&_SPIFFS_start - 0x40200000) / SPI_FLASH_SEC_SIZE), HEX);
  DPRINTP(" (");
  DPRINT(((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start)/1024, DEC);
  DPRINTPLN("K)");
  // 0x40200000 ist der Speicherort des SPI FLASH in der Memory Map

  String fileName;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    IPRINTP("FS: ");
    //fileName = dir.fileName();
    //size_t fileSize = dir.fileSize();
    //DPRINTF("[INFO]\tFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    DPRINT(dir.fileName());
    File f = dir.openFile("r");
    DPRINTP("\t: ");
    DPRINTLN(formatBytes(f.size()));
  }

  FSInfo fs_info;
  SPIFFS.info(fs_info);
  IPRINTP("Total: ");
  DPRINT(formatBytes(fs_info.totalBytes));
  DPRINTP("\tUsed: ");
  DPRINTLN(formatBytes(fs_info.usedBytes));

  //u32_t total, used;
  //int res = SPIFFS_info(&fs, &total, &used);
  
  // WIFI
  if (!loadconfig(eWIFI,0)) {
    serialNote(WIFI_FILE,0);
    setconfig(eWIFI,{});  // Speicherplatz vorbereiten
  } else serialNote(WIFI_FILE,1);

  // SYSTEM
  if (!loadconfig(eSYSTEM,0)) {
    serialNote(SYSTEM_FILE,0);
    set_system();
    setconfig(eSYSTEM,{});  // Speicherplatz vorbereiten
    ESP.restart();
  } else serialNote(SYSTEM_FILE,1);
  
  // CHANNEL
  if (!loadconfig(eCHANNEL,0)) {
    if (!loadconfig(eCHANNEL,1)) {  // Alte Speicherwerte checken
      serialNote(CHANNEL_FILE,0);   // Reset der Werte
      set_channels(1);
      setconfig(eCHANNEL,{});  // Speicherplatz vorbereiten
      ESP.restart();
    }
  } else serialNote(CHANNEL_FILE,1);
  
  // IOT
  if (!loadconfig(eTHING,0)) {
    if (!loadconfig(eTHING,1)) {
      serialNote(THING_FILE,0);
      set_iot(0);
      setconfig(eTHING,{});  // Speicherplatz vorbereiten
    }
  } else serialNote(THING_FILE,1);

  // PITMASTER
  if (!loadconfig(ePIT,0)) {
    if (!loadconfig(ePIT,1)) {
      serialNote(PIT_FILE,0);
      set_pid(0);  // Default PID-Settings
      set_pitmaster(1);
      setconfig(ePIT,{});  // Reset pitmaster config
    }
  } else serialNote(PIT_FILE,1);

}


/*
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Test zum Speichern von Datalog

//unsigned char meinsatz[64] = "Ich nutze ab jetzt den Flash Speicher für meine Daten!\n";
//unsigned char meinflash[64];

void write_flash(uint32_t _sector) {

  noInterrupts();
  if(spi_flash_erase_sector(_sector) == SPI_FLASH_RESULT_OK) {  // ESP.flashEraseSector
    spi_flash_write(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) mylog, sizeof(mylog));  //ESP.flashWrite
    //DPRINTP("[LOG]\tSpeicherung im Sector: ");
    //DPRINTLN(_sector, HEX);
  } //else DPRINTPLN("[INFO]\tFehler beim Speichern im Flash");
  interrupts(); 
}


void read_flash(uint32_t _sector) {

  noInterrupts();
  spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) archivlog, sizeof(archivlog));  //ESP.flashRead
  //spi_flash_read(_sector * SPI_FLASH_SEC_SIZE, (uint32 *) meinflash, sizeof(meinflash));
  interrupts();
}

*/



