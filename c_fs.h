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

#define CHANNEL_FILE "/channel.json"
#define WIFI_FILE "/wifi.json"
#define THING_FILE "/thing.json"
#define PIT_FILE "/pit.json"
#define SYSTEM_FILE "/system.json"


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
      if (!checkjson(json,CHANNEL_FILE)) return false;
      if (json["VERSION"] != CHANNELJSONVERSION) return false;
  
      const char* author = json["AUTHOR"];
      temp_unit = json["temp_unit"].asString();

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
      std::unique_ptr<char[]> buf(new char[300]);
      readEE(buf.get(),300, 0);

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
  
    case 2:     // THINGSPEAK
    { 
      std::unique_ptr<char[]> buf(new char[100]);
      readEE(buf.get(),100, 300);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,THING_FILE)) return false;
      THINGSPEAK_KEY = json["KEY"].asString();
    }
    break;

    case 3:     // PITMASTER
    {
      std::unique_ptr<char[]> buf(new char[600]);
      readEE(buf.get(),600, 900);

      JsonArray& _pid = jsonBuffer.parseArray(buf.get());
      if (!checkjson(_pid,PIT_FILE)) return false;

      pidsize = 0;
      
      // Wie viele Pitmaster sind vorhanden
      for (JsonArray::iterator it=_pid.begin(); it!=_pid.end(); ++it) {  
        pid[pidsize].name = _pid[pidsize]["name"].asString();
        pid[pidsize].Kp = _pid[pidsize]["Kp"];  
        pid[pidsize].Ki = _pid[pidsize]["Ki"];    
        pid[pidsize].Kd = _pid[pidsize]["Kd"];                     
        pid[pidsize].Kp_a = _pid[pidsize]["Kp_a"];                   
        pid[pidsize].Ki_a = _pid[pidsize]["Ki_a"];                   
        pid[pidsize].Kd_a = _pid[pidsize]["Kd_a"];                   
        pid[pidsize].Ki_min = _pid[pidsize]["Ki_min"];                   
        pid[pidsize].Ki_max = _pid[pidsize]["Ki_max"];                  
        pid[pidsize].pswitch = _pid[pidsize]["switch"];               
        pid[pidsize].pause = _pid[pidsize]["pause"];                   
        pid[pidsize].freq = _pid[pidsize]["freq"];
        pid[pidsize].esum = 0;             
        pid[pidsize].elast = 0;       
        pidsize++;
      }
    }
    break;

    case 4:     // SYSTEM
    {
      std::unique_ptr<char[]> buf(new char[250]);
      readEE(buf.get(),250, 1500);
      
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (!checkjson(json,SYSTEM_FILE)) return false;
  
      const char* author = json["AUTHOR"];
      if (json.containsKey("host")) host = json["host"].asString();
      else return false;
      if (json.containsKey("hwalarm")) doAlarm = json["hwalarm"];
      else return false;
      if (json.containsKey("lang")) language = json["lang"].asString();
      else return false;
      if (json.containsKey("utc")) timeZone = json["utc"];
      else return false;
      if (json.containsKey("batmax")) battery.max = json["batmax"];
      else return false;
      if (json.containsKey("batmin")) battery.min = json["batmin"];
      else return false;
      if (json.containsKey("logsec")) {
        int sector = json["logsec"];
        if (sector > log_sector) log_sector = sector;
        // oberes limit wird spaeter abgefragt
      }
      else return false;
    }
    break;
    
    //case 5:     // PRESETS
    //break;
  
    default:
    return false;
  
  }

  /*
  DPRINT("[JSON GET]\t");
  json.printTo(Serial);
  DPRINTLN();
  */
  return true;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Set xxx.json
bool setconfig(byte count, const char* data[2]) {
  
  DynamicJsonBuffer jsonBuffer;
  File configFile;

  switch (count) {
    case 0:         // CHANNEL
    {
      JsonObject& json = jsonBuffer.createObject();
  
      json["AUTHOR"] = "s.ochs";
      json["VERSION"] = CHANNELJSONVERSION;
      json["temp_unit"] = temp_unit;

      JsonArray& _name = json.createNestedArray("tname");
      JsonArray& _typ = json.createNestedArray("ttyp");
      JsonArray& _min = json.createNestedArray("tmin");
      JsonArray& _max = json.createNestedArray("tmax");
      JsonArray& _alarm = json.createNestedArray("talarm");
      JsonArray& _color = json.createNestedArray("tcolor");
  
      for (int i=0; i < CHANNELS; i++){
        _name.add("Kanal " + String(i+1));
        _typ.add(0);
    
        if (temp_unit == "F") {
          _min.add(ULIMITMINF,1);
          _max.add(OLIMITMINF,1);
        } else {
          _min.add(ULIMITMIN,1);
          _max.add(OLIMITMIN,1);
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
      THINGSPEAK_KEY = data[0];
      json["KEY"] = THINGSPEAK_KEY;
      
      size_t size = json.measureLength() + 1;
      if (size > 100) {
        DPRINTPLN("[INFO]\tZu viele THINGSPEAK Daten!");
        return false;
      } else {
        clearEE(100,300);  // Bereich reinigen
        static char buffer[100];
        json.printTo(buffer, size);
        writeEE(buffer, size, 300);
      }
    }
    break;

    case 3:        // PITMASTER
    {
      JsonArray& json = jsonBuffer.createArray();
      JsonObject& _pid = json.createNestedObject();

      // Default Pitmaster
      pid[0] = {"SSR", 3.8, 0.01, 128, 6.2, 0.001, 5, 0, 95, 0.9, 1000, 0, 0, 0};
      pidsize = 0;  // Reset counter
      
      _pid["name"]    = pid[pidsize].name;
      _pid["Kp"]      = pid[pidsize].Kp;  
      _pid["Ki"]      = pid[pidsize].Ki;    
      _pid["Kd"]      = pid[pidsize].Kd;                   
      _pid["Kp_a"]    = pid[pidsize].Kp_a;               
      _pid["Ki_a"]    = pid[pidsize].Ki_a;                  
      _pid["Kd_a"]    = pid[pidsize].Kd_a;             
      _pid["Ki_min"]  = pid[pidsize].Ki_min;             
      _pid["Ki_max"]  = pid[pidsize].Ki_max;             
      _pid["switch"]  = pid[pidsize].pswitch;           
      _pid["pause"]   = pid[pidsize].pause;                
      _pid["freq"]    = pid[pidsize].freq;
      pidsize++;
      
      size_t size = json.measureLength() + 1;
      if (size > 600) {
        DPRINTPLN("[INFO]\tZu viele PITMASTER Daten!");
        return false;
      } else {
        clearEE(600,900);  // Bereich reinigen
        static char buffer[600];
        json.printTo(buffer, size);
        writeEE(buffer, size, 900);
      }
    }
    break;

    case 4:         // SYSTEM
    {
      String host = HOSTNAME;
      host += String(ESP.getChipId(), HEX);
      
      JsonObject& json = jsonBuffer.createObject();
  
      json["AUTHOR"] = "s.ochs";
      json["host"] = host;
      json["hwalarm"] = false;    // doAlarm
      json["ap"] = APNAME;
      json["lang"] = "de";
      json["utc"] = 1;
      json["batmax"] = BATTMAX;
      json["batmin"] = BATTMIN;
      json["logsec"] = log_sector;
    
      size_t size = json.measureLength() + 1;
      clearEE(250,1500);  // Bereich reinigen
      static char buffer[250];
      json.printTo(buffer, size);
      writeEE(buffer, size, 1500);
    }
    break;

    case 5:         //PRESETS
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
bool modifyconfig(byte count, const char* data[12]) {
  
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
      if (!checkjson(alt,CHANNEL_FILE)) return false;
      
      // Neue Daten erzeugen
      JsonObject& json = jsonBuffer.createObject();

      json["AUTHOR"] = alt["AUTHOR"];
      json["VERSION"] = CHANNELJSONVERSION;
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

      // Speichern
      //if (!savefile(CHANNEL_FILE, configFile)) return false;
      //json.printTo(configFile);
      //configFile.close();
      size_t size = json.measureLength() + 1;
      clearEE(500,400);  // Bereich reinigen
      static char buffer[500];
      json.printTo(buffer, size);
      writeEE(buffer, size, 400);

      //DPRINT("[JSON SET]\t");
      //json.printTo(Serial);
      //DPRINTLN();
      
    }
    break;

    case 1:         // WIFI
    {
      // Alte Daten auslesen
      std::unique_ptr<char[]> buf(new char[300]);
      readEE(buf.get(), 300, 0);

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
      
      if (size > 300) {
        DPRINTPLN("[INFO]\tZu viele WIFI Daten!");
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

    case 3:         // PITMASTER
    {
      // Alte Daten auslesen
      std::unique_ptr<char[]> buf(new char[600]);
      readEE(buf.get(), 600, 900);

      JsonArray& json = jsonBuffer.parseArray(buf.get());
      if (!checkjson(json,PIT_FILE)) {
        setconfig(ePIT,{});
        return false;
      }

      // Neue Daten eintragen
      JsonObject& _pid = json.createNestedObject();
      
      _pid["name"]    = pid[pidsize].name;
      _pid["Kp"]      = pid[pidsize].Kp;  
      _pid["Ki"]      = pid[pidsize].Ki;    
      _pid["Kd"]      = pid[pidsize].Kd;                   
      _pid["Kp_a"]    = pid[pidsize].Kp_a;               
      _pid["Ki_a"]    = pid[pidsize].Ki_a;                  
      _pid["Kd_a"]    = pid[pidsize].Kd_a;             
      _pid["Ki_min"]  = pid[pidsize].Ki_min;             
      _pid["Ki_max"]  = pid[pidsize].Ki_max;             
      _pid["switch"]  = pid[pidsize].pswitch;           
      _pid["pause"]   = pid[pidsize].pause;                
      _pid["freq"]    = pid[pidsize].freq;
    
      // Speichern
      size_t size = json.measureLength() + 1;
      if (size > 600) {
        DPRINTPLN("[INFO]\tZu viele PITMASTER Daten!");
        return false;
      } else {
        static char buffer[600];
        json.printTo(buffer, size);
        writeEE(buffer, size, 900); 
      } 
    }
    break;

    case 4:           // SYSTEM
    {
      // Alte Daten auslesen
      std::unique_ptr<char[]> buf(new char[250]);
      readEE(buf.get(),250, 1500);
      
      JsonObject& alt = jsonBuffer.parseObject(buf.get());
      if (!checkjson(alt,SYSTEM_FILE)) return false;
      
      // Neue Daten erzeugen
      JsonObject& json = jsonBuffer.createObject();

      json["AUTHOR"] = alt["AUTHOR"];
      //json["VERSION"] = CHANNELJSONVERSION;
            
      json["host"] = host;
      json["hwalarm"] = doAlarm;
      json["ap"] = APNAME;
      json["lang"] = language;
      json["utc"] = timeZone;
      json["batmax"] = battery.max;
      json["batmin"] = battery.min;
      json["logsec"] = log_sector;

      // Speichern
      size_t size = json.measureLength() + 1;
      clearEE(250,1500);  // Bereich reinigen
      static char buffer[250];
      json.printTo(buffer, size);
      writeEE(buffer, size, 1500);
      
    }
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
    DPRINTPLN("[INFO]\tFailed to mount file system");
    return;
  }

  DPRINTP("[INFO]\tInitalize SPIFFS at Sector: 0x");
  DPRINT((((uint32_t)&_SPIFFS_start - 0x40200000) / SPI_FLASH_SEC_SIZE), HEX);
  DPRINTP(" (");
  DPRINT(((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start)/1024, DEC);
  DPRINTPLN("K)");
  // 0x40200000 ist der Speicherort des SPI FLASH in der Memory Map

  // CHANNEL
  if (!loadconfig(eCHANNEL)) {
    DPRINTPLN("[INFO]\tFailed to load channel config");
    setconfig(eCHANNEL,{});  // Speicherplatz vorbereiten
    ESP.restart();
  } else DPRINTPLN("[INFO]\tChannel config loaded");


  // WIFI
  if (!loadconfig(eWIFI)) {
    DPRINTPLN("[INFO]\tFailed to load wifi config");
    setconfig(eWIFI,{});  // Speicherplatz vorbereiten
  } else DPRINTPLN("[INFO]\tWifi config loaded");


  // THINGSPEAK
  if (!loadconfig(eTHING)) DPRINTPLN("[INFO]\tFailed to load Thingspeak config");
  else DPRINTPLN("[INFO]\tThingspeak config loaded");


  // PITMASTER
  if (!loadconfig(ePIT)) {
    DPRINTPLN("[INFO]\tFailed to load pitmaster config");
    setconfig(ePIT,{});  // Reset pitmaster config
  } else DPRINTPLN("[INFO]\tPitmaster config loaded");


  // SYSTEM
  if (!loadconfig(eSYSTEM)) {
    DPRINTPLN("[INFO]\tFailed to load system config");
    setconfig(eSYSTEM,{});  // Speicherplatz vorbereiten
    ESP.restart();
  } else DPRINTPLN("[INFO]\tSystem config loaded");

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

void getLog(StreamString *output,int maxlog) {

  //StreamString output;

  int logstart;
  int logend;
  int rest = log_count%MAXLOGCOUNT;
  int saved = (log_count-rest)/MAXLOGCOUNT;    // Anzahl an gespeicherten Sektoren
  
  if (log_count < MAXLOGCOUNT+1) {             // noch alle Daten im Kurzspeicher
    logstart = 0;
    logend = log_count;
  } else {                                    // Daten aus Kurzspeicher und Archiv

    saved = constrain(saved, 0, maxlog);      // maximal angezeigte Logdaten
    int savedend = saved;
    
    if (rest == 0) {                          // noch ein Logpaket im Kurzspeicher
      logstart = 0;                           
      savedend--;
    } else  logstart = MAXLOGCOUNT - rest;   // nur Rest aus Kurzspeicher holen
    
    logend = MAXLOGCOUNT;

    for (int k = 0; k < savedend; k++) {
      Serial.println(log_sector - saved + k,HEX);

      /*
      read_flash(log_sector - saved + k);

      for (int j = 0; j < MAXLOGCOUNT; j++) {
        for (int i=0; i < CHANNELS; i++)  {
          output->print(archivlog[j].tem[i]/10.0);
          output->print(";");
        }
        output->print(archivlog[j].pitmaster);
        output->print(";");
        output->print(digitalClockDisplay(archivlog[j].timestamp));
        output->print("\r\n");
      }
      output->print("\r\n");
      */
    }
  }

  // Kurzspeicher auslesen
  for (int j = logstart; j < logend; j++) {
    for (int i=0; i < CHANNELS; i++)  {
      output->print(mylog[j].tem[i]/10.0);
      output->print(";");
    }
    output->print(mylog[j].pitmaster);
    output->print(";");
    output->print(digitalClockDisplay(mylog[j].timestamp));
    output->print("\r\n");
  }

    /* Test

        read_flash(log_sector-1);
        for (int j=0; j<10; j++) {
          int16_t test = archivlog[j].tem[0];
          Serial.print(test/10.0);
          Serial.print(" ");
        }
    */

   
  //Serial.print(output);
    

    
}

void getLog(File *output,int maxlog) {

  int logstart;
  int logend;
  int rest = log_count%MAXLOGCOUNT;
  int saved = (log_count-rest)/MAXLOGCOUNT;    // Anzahl an gespeicherten Sektoren
  
  if (log_count < MAXLOGCOUNT+1) {             // noch alle Daten im Kurzspeicher
    logstart = 0;
    logend = log_count;
  } else {                                    // Daten aus Kurzspeicher und Archiv

    saved = constrain(saved, 0, maxlog);      // maximal angezeigte Logdaten
    int savedend = saved;
    
    if (rest == 0) {                          // noch ein Logpaket im Kurzspeicher
      logstart = 0;                           
      savedend--;
    } else  logstart = MAXLOGCOUNT - rest;   // nur Rest aus Kurzspeicher holen
    
    logend = MAXLOGCOUNT;

    for (int k = 0; k < savedend; k++) {
      Serial.println(log_sector - saved + k,HEX);

      /*
      read_flash(log_sector - saved + k);

      for (int j = 0; j < MAXLOGCOUNT; j++) {
        for (int i=0; i < CHANNELS; i++)  {
          output->print(archivlog[j].tem[i]/10.0);
          output->print(";");
        }
        output->print(archivlog[j].pitmaster);
        output->print(";");
        output->print(digitalClockDisplay(archivlog[j].timestamp));
        output->print("\r\n");
      }
      output->print("\r\n");
      */
      
    }
  }

  // Kurzspeicher auslesen
  for (int j = logstart; j < logend; j++) {
    for (int i=0; i < CHANNELS; i++)  {
      output->print(mylog[j].tem[i]/10.0);
      output->print(";");
    }
    output->print(mylog[j].pitmaster);
    output->print(";");
    output->print(digitalClockDisplay(mylog[j].timestamp));
    output->print("\r\n");
  }
    
}




