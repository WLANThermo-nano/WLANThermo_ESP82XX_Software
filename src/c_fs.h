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
#define PUSH_FILE     "/push.json"
#define SERVER_FILE   "/url.json"
#define CLOUD_FILE    "/cloud.json"


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
  DPRINTP("[INFO]\tSaved: ");
  DPRINTLN(filename);
  return true;
}

#ifdef MEMORYCLOUD
void saveLog() {
  
  // CHANNEL
  for (int i = 0; i < sys.ch; i++) {
    cloudlog[cloudcount].tem[i] = limit_float(ch[i].temp, i)*10;
    //cloudlog[cloudcount].color[i] = ch[i].color;
  }

  // PITMASTER
  cloudlog[cloudcount].value = (int)pitMaster[0].value;
  cloudlog[cloudcount].set = pitMaster[0].set*10;
  cloudlog[cloudcount].status = pitMaster[0].active;

  // SYSTEM
  cloudlog[cloudcount].soc = battery.percentage;
  cloudcount++;  
}

void parseLog(JsonObject  &jObj, byte c, long tim) {

  JsonObject& system = jObj.createNestedObject("system");

  system["time"] = String(tim);
  system["soc"] = cloudlog[c].soc;
  
  JsonArray& channel = jObj.createNestedArray("channel");

  for (int i = 0; i < sys.ch; i++) {
    JsonObject& data = channel.createNestedObject();
    data["number"]= i+1;
    data["temp"]  = cloudlog[c].tem[i]/10.0;
    //data["color"] = cloudlog[c].color[i];
  }

  JsonObject& master = jObj.createNestedObject("pitmaster");

  master["value"] = cloudlog[c].value;
  master["set"] = cloudlog[c].set/10.0;
  switch (cloudlog[c].status) {
    case PITOFF:   master["typ"] = "off";    break;
    case DUTYCYCLE: // show manual
    case MANUAL:   master["typ"] = "manual"; break;
    case AUTO:     master["typ"] = "auto";   break;
    case AUTOTUNE: master["typ"] = "autotune"; break;
  }
}
#endif

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

  const size_t bufferSize = 6*JSON_ARRAY_SIZE(MAXCHANNELS) + JSON_OBJECT_SIZE(9) + 320;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  File configFile;

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
      
      for (int i=0; i < sys.ch; i++){
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

      #ifdef THINGSPEAK
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
      #endif
      
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
        //pid[pidsize].Kp_a = _pid[pidsize]["Kp_a"];                   
        //pid[pidsize].Ki_a = _pid[pidsize]["Ki_a"];                   
        //pid[pidsize].Kd_a = _pid[pidsize]["Kd_a"];                   
        //pid[pidsize].Ki_min = _pid[pidsize]["Ki_min"];                   
        //pid[pidsize].Ki_max = _pid[pidsize]["Ki_max"];                  
        //pid[pidsize].pswitch = _pid[pidsize]["switch"];                              
        pid[pidsize].DCmin    = _pid[pidsize]["DCmin"];              
        pid[pidsize].DCmax    = _pid[pidsize]["DCmax"];              
        //pid[pidsize].SVmin    = _pid[pidsize]["SVmin"];             
        //pid[pidsize].SVmax    = _pid[pidsize]["SVmax"];  
        pid[pidsize].jumppw  =  _pid[pidsize]["jp"];  
        pid[pidsize].opl  =  _pid[pidsize]["ol"];     

        if (pid[pidsize].jumppw == 0) pid[pidsize].jumppw = 100;      // Übergang von alten Versionen
        
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
  
      if (json.containsKey("ch"))       sys.ch = json["ch"];
      else sys.ch = MAXCHANNELS;
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
     
      if (json.containsKey("hwversion")) sys.hwversion = json["hwversion"];
      else return false;
      if (json.containsKey("update"))   update.state = json["update"];
      else return false;
      if (json.containsKey("autoupd"))  update.autoupdate = json["autoupd"];
      else return false;
      if (json.containsKey("getupd"))   update.get = json["getupd"].asString();
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


      if (update.state == 3) update.state = 4;     // Ende eines Updates über alte API (v0.x.x)
  
      
    }
    break;

    case 5:     // PUSH
    { 
      std::unique_ptr<char[]> buf(new char[EEPUSH]);
      readEE(buf.get(),EEPUSH, EEPUSHBEGIN);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,PUSH_FILE)) return false;
      
      if (json.containsKey("onP"))      pushd.on = json["onP"];
      else return false;
      if (json.containsKey("tokP"))     pushd.token = json["tokP"].asString();
      else return false;
      if (json.containsKey("idP"))      pushd.id = json["idP"].asString(); 
      else return false;
      if (json.containsKey("rptP"))     pushd.repeat = json["rptP"];
      else return false;
      if (json.containsKey("svcP"))     pushd.service = json["svcP"];
      else return false;
    }
    break;

    case 6:     // SERVERURL
    {
      if (!loadfile(SERVER_FILE,configFile)) return false;
      std::unique_ptr<char[]> buf(new char[configFile.size()]);
      configFile.readBytes(buf.get(), configFile.size());
      configFile.close();
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,SERVER_FILE)) return false;

      for (int i = 0; i < NUMITEMS(serverurl); i++) {
        
        JsonObject& _link = json[serverurl[i].typ];

        if (_link.containsKey("host")) serverurl[i].host = _link["host"].asString();
        else return false;
        
        if (_link.containsKey("page")) serverurl[i].page = _link["page"].asString();
        //else return false;
      }
    }
   break;
    
    //case 7:     // PRESETS
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
  File configFile;

  #ifdef MEMORYCLOUD
  cloudcount = 0;     // Zurücksetzen
  #endif

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
    
      for (int i=0; i < sys.ch; i++){
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

      #ifdef THINGSPEAK
      json["TSwrite"] = iot.TS_writeKey;
      json["TShttp"]  = iot.TS_httpKey;
      json["TSuser"]  = iot.TS_userKey;
      json["TSchID"]  = iot.TS_chID;
      json["TS8"]     = iot.TS_show8;
      json["TSint"]   = iot.TS_int;
      json["TSon"]    = iot.TS_on;
      #endif
      json["PMQhost"]  = iot.P_MQTT_HOST;
      json["PMQport"]  = iot.P_MQTT_PORT;
      json["PMQuser"]  = iot.P_MQTT_USER;
      json["PMQpass"]  = iot.P_MQTT_PASS;
      json["PMQqos"]   = iot.P_MQTT_QoS;
      json["PMQon"]   = iot.P_MQTT_on;
      json["PMQint"]   = iot.P_MQTT_int;
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
        //_pid["Kp_a"]     = double_with_n_digits(pid[i].Kp_a,1);               
        //_pid["Ki_a"]     = double_with_n_digits(pid[i].Ki_a,3);                  
        //_pid["Kd_a"]     = double_with_n_digits(pid[i].Kd_a,1);             
        //_pid["Ki_min"]   = pid[i].Ki_min;             
        //_pid["Ki_max"]   = pid[i].Ki_max;             
        //_pid["switch"]   = pid[i].pswitch;                           
        _pid["DCmin"]    = double_with_n_digits(pid[i].DCmin,1);             
        _pid["DCmax"]    = double_with_n_digits(pid[i].DCmax,1);             
        _pid["jp"]    = pid[i].jumppw;  
        //_pid["SVmin"]    = pid[i].SVmin;             
        //_pid["SVmax"]    = pid[i].SVmax;
        _pid["ol"]    = pid[i].opl;   
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

      json["ch"] =          sys.ch;
      json["host"] =        sys.host;
      json["ap"] =          sys.apname;
      json["lang"] =        sys.language;
      json["hwversion"] =   sys.hwversion;
      json["update"] =      update.state;
      json["getupd"] =      update.get;
      json["autoupd"] =     update.autoupdate;
      json["batmax"] =      battery.max;
      json["batmin"] =      battery.min;
      json["god"] =         sys.god;
      json["typk"] =        sys.typk;
      json["pitsup"] =      sys.pitsupply;
      json["batfull"] =     battery.setreference;
      json["pass"] =        sys.www_password;
      json["damper"] =      sys.damper;
    
      size_t size = json.measureLength() + 1;
      clearEE(EESYSTEM,EESYSTEMBEGIN);  // Bereich reinigen
      static char buffer[EESYSTEM];
      json.printTo(buffer, size);
      writeEE(buffer, size, EESYSTEMBEGIN);
    }
    break;

    case 5:         //PUSH
    {
      JsonObject& json = jsonBuffer.createObject();
      
      json["onP"]    = pushd.on;
      json["tokP"]   = pushd.token;
      json["idP"]    = pushd.id;
      json["rptP"]   = pushd.repeat;
      json["svcP"]   = pushd.service;
      
      size_t size = json.measureLength() + 1;
      if (size > EEPUSH) {
        IPRINTPLN("f:full");
        return false;
      } else {
        clearEE(EEPUSH,EEPUSHBEGIN);  // Bereich reinigen
        static char buffer[EEPUSH];
        json.printTo(buffer, size);
        writeEE(buffer, size, EEPUSHBEGIN);
      }
    }
    break;

    case 6:         //SERVERLINK
    {
      JsonObject& json = jsonBuffer.createObject();

      for (int i = 0; i < NUMITEMS(serverurl); i++) {
  
        JsonObject& _obj = json.createNestedObject(serverurl[i].typ);
        _obj["host"] =  serverurl[i].host;
        _obj["page"] =  serverurl[i].page;
      }
      if (!savefile(SERVER_FILE, configFile)) return false;
      json.printTo(configFile);
      configFile.close();
    }
    break;

    case 7:         //PRESETS
    {
      
    }
    break;

    default:
    return false;
  
  }
  if (pmqttClient.connected()) sys.sendSettingsflag = true;
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
      Serial.print(wifi.savedlen);
      
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

    case 5:           // PUSH
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

  // PUSH
  if (!loadconfig(ePUSH,0)) {
    serialNote(PUSH_FILE,0);
    set_push();
    setconfig(ePUSH,{});  // Speicherplatz vorbereiten
  } else serialNote(PUSH_FILE,1);

  // SERVER
  if (!loadconfig(eSERVER,0)) {
    serialNote(SERVER_FILE,0);
    setserverurl();
    setconfig(eSERVER,{});  // Speicherplatz vorbereiten
  } else serialNote(SERVER_FILE,1);
  

  IPRINTP("M24C02: ");
  if (m24.exist()) {
    DPRINTP("0x");
    DPRINTLN(m24.getadress(), HEX);
    
    char item[PRODUCTNUMBERLENGTH];    // item ist 10 Zeichen + Schlusszeichen
    m24.get(0,item);
    if (item[0] == 'n') {   // Kennung
      String str(item);
      sys.item = str;
    }

    if (m24.getadress() == 0x52) {
      if (sys.hwversion != 2) {
        sys.hwversion = 2;
        sys.pitsupply = true;
        setconfig(eSYSTEM,{});
        IPRINTPLN("Umstellung auf V1+");
      }
      if (item[9] == '0') {
        sys.pitmaster = false;
        IPRINTPLN("Kein Pitmaster");
      }
    } else if (m24.getadress() == 0x51) {
      if (item[10] == 'k' && !sys.typk) {
        sys.typk = true;
        set_sensor();
        setconfig(eSYSTEM,{});  // Speichern
        IPRINTPLN("Umstellung auf Typ K");
      }
    }
  } else DPRINTPLN("No");

}




