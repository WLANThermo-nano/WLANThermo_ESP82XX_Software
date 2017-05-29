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

AsyncWebServer server(80);        // https://github.com/me-no-dev/ESPAsyncWebServer

const char* www_username = "admin";
const char* www_password = "admin";

// Beispiele:
// https://github.com/spacehuhn/wifi_ducky/blob/master/esp8266_wifi_duck/esp8266_wifi_duck.ino



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
String getContentType(String filename, AsyncWebServerRequest *request) {
  if (request->hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleFileRead(String path, AsyncWebServerRequest *request){
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path, request);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
      DPRINTLN(path);
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);
      if (path.endsWith(".gz"))
        response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    
    return true;
  }
  return false;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void handleSettings(AsyncWebServerRequest *request, bool www) {

  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server","ESP Async Web Server");
  
  JsonObject& root = response->getRoot();
  JsonObject& _system = root.createNestedObject("system");

  _system["time"] = String(now());
  _system["utc"] = timeZone;
  _system["ap"] = APNAME;
  _system["host"] = host;
  _system["language"] = language;
  _system["unit"] = temp_unit;
  _system["hwalarm"] = doAlarm;
  _system["version"] = FIRMWAREVERSION;
  
  JsonArray& _typ = root.createNestedArray("sensors");
  for (int i = 0; i < SENSORTYPEN; i++) {
    _typ.add(ttypname[i]);
  }

  JsonArray& _pit = root.createNestedArray("pitmaster");
  for (int j = 0; j < pidsize; j++) {
    _pit.add(pid[j].name);
  }

  JsonObject& _chart = root.createNestedObject("charts");
  _chart["thingspeak"] = THINGSPEAK_KEY;
    
  if (www) {
    response->setLength();
    request->send(response);
  } else root.printTo(Serial);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void handleData(AsyncWebServerRequest *request, bool www) {

  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server","ESP Async Web Server");
  
  JsonObject& root = response->getRoot();
  JsonObject& system = root.createNestedObject("system");

  system["time"] = String(now());
  system["utc"] = timeZone;
  system["soc"] = battery.percentage;
  system["charge"] = !battery.charge;
  system["rssi"] = rssi;
  system["unit"] = temp_unit;

  JsonArray& channel = root.createNestedArray("channel");

  for (int i = 0; i < CHANNELS; i++) {
  JsonObject& data = channel.createNestedObject();
    int ctemp = ch[i].temp * 10;
    data["number"]= i+1;
    data["name"]  = ch[i].name;
    data["typ"]   = ch[i].typ;
    data["temp"]  = (float) ctemp/10.0;
    data["min"]   = ch[i].min;
    data["max"]   = ch[i].max;
    data["alarm"] = ch[i].alarm;
    data["color"] = ch[i].color;
  }
  
  JsonObject& master = root.createNestedObject("pitmaster");

  master["channel"] = pitmaster.channel;
  master["typ"] = pitmaster.typ;
  master["value"] = pitmaster.value;
  master["set"] = pitmaster.set;
  master["active"] = pitmaster.active;
  
  if (www) {
    response->setLength();
    request->send(response);
  } else root.printTo(Serial);
}

/*
// Funktioniert leider nur bis zu einer gewissen Laenge (2741 Zeichen)
void handleLog(AsyncWebServerRequest *request, bool www) {

  StreamString output;
  getLog(&output,0);
  request->send(output, "application/json", output.length());
}
*/

/*
void handlePlot(AsyncWebServerRequest *request, bool www) {
    //unsigned long ulSizeList = MakeList(&client,false); // get size of list first

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    
    response->print(F("<html>\n<head>\n<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n"));
    response->print(F("<script type=\"text/javascript\">\n google.charts.load('current', {packages: ['corechart', 'line']});\ngoogle.charts.setOnLoadCallback(drawChart);\n"));
    response->print(F("function drawChart() {\nvar data = new google.visualization.DataTable();\ndata.addColumn('datetime','name');\ndata.addColumn('number','Pitmaster');\ndata.addColumn('number','Soll');\n"));
    response->print(F("data.addColumn('number','Kanal1');\n"));
    response->print(F("data.addRows([\n"));

    int sta = (log_count > 20)?log_count-20:0;
    
    for (int j=sta; j < log_count; j++) {

      response->print("[");
      response->print(newDate(mylog[j].timestamp));
      response->print(",");
      response->print(mylog[j].pitmaster);
      response->print(",100");
      
      for (int i=0; i < 1; i++)  {
        response->print(",");
        if (mylog[j].tem[i]/10 == INACTIVEVALUE)
          response->print(0);
        else
          response->print(mylog[j].tem[i]/10.0);   
      } 
      response->print("],\n");
    }

    // part 2 of response - after the big list
    response->print(F("]);\nvar options = {\nhAxis: {gridlines: {color: 'white', count: 5}},\nwidth: 700,\nheight: 400,\ncurveType: 'function',\n"));
    response->print(F("vAxes:{\n0:{title:'Pitmaster in %',ticks:[0,20,40,60,80,100],viewWindow:{min: 0},gridlines:{color:'transparent'},titleTextStyle:{italic:0}},\n1: {title: 'Temperatur in C',maxValue: 120, minValue: 0, gridlines: {color: 'transparent'}, titleTextStyle: {italic:0}},},\n"));
    response->print(F("series:{\n0: {targetAxisIndex: 0, color: 'black'},\n1: {targetAxisIndex: 1, color: 'red', lineDashStyle: [4,4]},\n2: {targetAxisIndex: 1, color: '#6495ED'},\n},\n};\n"));
    response->print(F("var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));\nchart.draw(data, options);}\n</script>\n</head>\n"));
    response->print(F("<body>\n<font color=\"#000000\">\n<body bgcolor=\"#d0d0f0\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">"));
    response->print(F("\n<div id=\"curve_chart\"></div>\n</body>\n</html>"));
    
    // Send the response to the client - delete strings after use to keep mem low
     
    
    //MakeList(&client,true);

    request->send(response);

}
*/
int dothis;

void handlePlot(AsyncWebServerRequest *request, bool www) {
    //unsigned long ulSizeList = MakeList(&client,false); // get size of list first

    dothis = (log_count<MAXLOGCOUNT)?log_count:MAXLOGCOUNT;
    
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    
    StreamString output;
    Serial.println(maxLen);
    
    if (index == 0) {
      Serial.print("first: ");
      output.print(F("<html>\n<head>\n<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n"));
      output.print(F("<script type=\"text/javascript\">\n google.charts.load('current', {packages: ['corechart', 'line']});\ngoogle.charts.setOnLoadCallback(drawChart);\n"));
      output.print(F("function drawChart() {\nvar data = new google.visualization.DataTable();\ndata.addColumn('datetime','name');\ndata.addColumn('number','Pitmaster');\ndata.addColumn('number','Soll');\n"));
      output.print(F("data.addColumn('number','Kanal1');\n"));
      output.print(F("data.addRows([\n"));
      
      Serial.println(output.length());
      
      if (output.length() < maxLen) {
        output.getBytes(buffer, maxLen); 
        return output.length();
      } else return 0;
      
    } else if (dothis > 0)  {


      while (output.length()+50 < maxLen && dothis != 0) {  
        
        int j;
        if (log_count<MAXLOGCOUNT) j = log_count - dothis;
        else j = MAXLOGCOUNT - dothis;     // ACHTUNG: log_count verändert sich, könnte blöd werden
        
        //Serial.print(log_count); Serial.print(" | ");
        //Serial.print(log_count%MAXLOGCOUNT); Serial.print(" | ");
        //Serial.println(dothis);
        
        output.print(F("["));
        output.print(newDate(mylog[j].timestamp));
        output.print(F(","));
        output.print(mylog[j].pitmaster);
        output.print(",100");
      
        for (int i=0; i < 1; i++)  {
          output.print(",");
          if (mylog[j].tem[i]/10 == INACTIVEVALUE)
            output.print(0);
          else
            output.print(mylog[j].tem[i]/10.0);   
        } 
        output.print("],\n");
        
        dothis--;
      }
      Serial.print("next: ");
      Serial.println(output.length());
      
      output.getBytes(buffer, maxLen); 
      return output.length();

    } else if (dothis == 0) {

      Serial.print("last: ");
      
      output.print(F("]);\nvar options = {\nhAxis: {gridlines: {color: 'white', count: 5}},\nwidth: 700,\nheight: 400,\ncurveType: 'function',\n"));
      output.print(F("vAxes:{\n0:{title:'Pitmaster in %',ticks:[0,20,40,60,80,100],viewWindow:{min: 0},gridlines:{color:'transparent'},titleTextStyle:{italic:0}},\n1: {title: 'Temperatur in C', minValue: 0, gridlines: {color: 'transparent'}, titleTextStyle: {italic:0}},},\n"));
      output.print(F("series:{\n0: {targetAxisIndex: 0, color: 'black'},\n1: {targetAxisIndex: 1, color: 'red', lineDashStyle: [4,4]},\n2: {targetAxisIndex: 1, color: '#6495ED'},\n},\n};\n"));
      output.print(F("var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));\nchart.draw(data, options);}\n</script>\n</head>\n"));
      output.print(F("<body>\n<font color=\"#000000\">\n<body bgcolor=\"#d0d0f0\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">"));
      output.print(F("\n<div id=\"curve_chart\"></div>\n</body>\n</html>"));

      Serial.println(output.length());
      dothis--;
      if (output.length() < maxLen) {
        output.getBytes(buffer, maxLen); 
        return output.length();
      } else  return 0;

      
    } else {
      dothis = 2;
      return 0;
    }
    

/*
    
    
    

    // part 2 of response - after the big list
    
    
    // Send the response to the client - delete strings after use to keep mem low
     
    
    //MakeList(&client,true);

*/
    });
    
    request->send(response);

}


void handleLog(AsyncWebServerRequest *request, bool www) {

  File f = SPIFFS.open("/log.html", "w");
  
  getLog(&f,3);
  
  f.close();
  
  request->send(SPIFFS, "/log.html", String(), true);
  
  SPIFFS.remove("/log.txt");
  Serial.println("LOG");

}


/*
void handleLog(AsyncWebServerRequest *request, bool www) {

  //StreamString output;
  //getLog(&output,0);

 // Serial.println(output.length());
  
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {

    // wenn in der lambda Funktion dann werden nur etwa 3,3kB übertragen
    StreamString output;
  getLog(&output,0);
  
    size_t Len = output.length();
    size_t leftToWrite = Len - index;
    
    if(! leftToWrite)
     return 0;//end of transfer
    size_t willWrite = (leftToWrite > maxLen)?maxLen:leftToWrite;

    for (int i = 0; i < willWrite; i++) {
      buffer[i] = output[index+i];
    }
    return willWrite;
  });
  

  // Als log.txt speichern
  char buf[26+7];
  snprintf(buf, sizeof (buf), "attachment; filename=\"log.txt\"");
  response->addHeader("Content-Disposition", buf);

  //response->addHeader("Server","ESP Async Web Server");
  request->send(response);
 
}
*/

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void handleLog2(AsyncWebServerRequest *request, bool www) {

  //StreamString output;
  //output.print("Hallo");



/* 
    StreamString output;
      if(output.reserve(8192)){

        for (int j=0; j < log_count; j++) {
          for (int i=0; i < CHANNELS; i++)  {
            if (mylog[j].tem[i]/10 == INACTIVEVALUE)
              output.print(0);
            else
              output.print(mylog[j].tem[i]/10.0);
            output.print(";");
          }
          output.print(mylog[j].pitmaster);
          output.print(";");
          output.print(digitalClockDisplay(mylog[j].timestamp));
          output.print("\r\n");
      }

      } else {
        output.print("Fehler");
      }
      request->send(200, "text/html", (String)output);
 
 *   
  AsyncResponseStream *response = request->beginResponseStream("text/html");
 
  for (int j=0; j < log_count; j++) {
      for (int i=0; i < CHANNELS; i++)  {
        if (mylog[j].tem[i]/10 == INACTIVEVALUE)
          response->print(0);
        else
          response->print(mylog[j].tem[i]/10.0);
        response->print(";");
      }
      response->print(mylog[j].pitmaster);
      response->print(";");
      response->print(digitalClockDisplay(mylog[j].timestamp));
      response->print("\r\n");
  }

  request->send(response);
  

  AsyncWebServerResponse *response = request->beginResponse("text/plain", 16, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
  //Write up to "maxLen" bytes into "buffer" and return the amount written.
  //index equals the amount of bytes that have been already sent
  //You will be asked for more data until 0 is returned
  //Keep in mind that you can not delay or yield waiting for more data!
  //buffer[i-startP] = char(EEPROM.read(i))
  return sendmore(buffer, maxLen);
  });
  
  response->addHeader("Server","ESP Async Web Server");
  request->send(response);
*/
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void handleWifiResult(AsyncWebServerRequest *request, bool www) {

  // https://github.com/me-no-dev/ESPAsyncWebServer/issues/85
 
  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server","ESP Async Web Server");

  JsonObject& json = response->getRoot();
  
  int n = WiFi.scanComplete();

  if (n > 0) {
  
    if (WiFi.status() == WL_CONNECTED)  {
      json["Connect"]   = true;
      json["Scantime"]  = millis()-scantime;
      json["SSID"]      = WiFi.SSID();
      json["IP"]        = WiFi.localIP().toString();
      json["Mask"]      = WiFi.subnetMask().toString();  
      json["Gate"]      = WiFi.gatewayIP().toString();
    }
    else {
      json["Connect"]   = false;
      json["SSID"]      = APNAME;
      json["IP"]        = WiFi.softAPIP().toString();
    }
  
    JsonArray& _scan = json.createNestedArray("Scan");
    for (int i = 0; i < n; i++) {
      JsonObject& _wifi = _scan.createNestedObject();
      _wifi["SSID"]   = WiFi.SSID(i);
      _wifi["RSSI"]   = WiFi.RSSI(i);
      _wifi["Enc"]    = WiFi.encryptionType(i);
      //_wifi["Hid"]  = WiFi.isHidden(i);
      if (WiFi.status() == WL_CONNECTED & WiFi.SSID(i) == WiFi.SSID()) {
        json["Enc"]       = WiFi.encryptionType(i);
        json["RSSI"]      = WiFi.RSSI(i);
      }
    }
  }
  
  if (www) {
    response->setLength();
    request->send(response);
  } else json.printTo(Serial);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void handleWifiScan(AsyncWebServerRequest *request, bool www) {

  //dumpClients();

  WiFi.scanDelete();
  if(WiFi.scanComplete() == -2){
    WiFi.scanNetworks(true);
    scantime = millis();

    if (www) request->send(200, "text/json", "OK");
    else Serial.println("OK");
  }   
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleSetChannels(AsyncWebServerRequest *request, uint8_t *datas) {

  //  https://github.com/me-no-dev/ESPAsyncWebServer/issues/123
  
  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& _cha = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_cha.success()) {
    return 0;
  }
    
  int num = _cha["number"];
  if (num > 0) {
    num--;          // Intern beginnt die Zählung bei 0
    String _name = _cha["name"].asString();                  // KANALNAME
    if (_name.length() < 11)  ch[num].name = _name;
    byte _typ = _cha["typ"];                                 // FÜHLERTYP
    if (_typ > -1 && _typ < SENSORTYPEN) ch[num].typ = _typ;  
    float _limit = _cha["min"];                              // LIMITS
    if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].min = _limit;
    _limit = _cha["max"];
    if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].max = _limit;
    ch[num].alarm = _cha["alarm"];                           // ALARM
    ch[num].color = _cha["color"].asString();                // COLOR
  }
  
  modifyconfig(eCHANNEL,{});                                      // SPEICHERN
  return 1;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleSetPitmaster(AsyncWebServerRequest *request, uint8_t *datas) {

  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _pitmaster = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_pitmaster.success()) return 0;
  
  pitmaster.channel = _pitmaster["channel"]; // 0
  pitmaster.typ = _pitmaster["typ"]; // ""
  pitmaster.set = _pitmaster["value"]; // 100
  pitmaster.active = _pitmaster["active"];
  return 1;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleSetNetwork(AsyncWebServerRequest *request, uint8_t *datas) {

  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _network = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_network.success()) return 0;

  if (!_network.containsKey("ssid")) return 0;
  holdssid.ssid = _network["ssid"].asString();
  if (!_network.containsKey("password")) return 0;
  holdssid.pass = _network["password"].asString();
  holdssid.connect = millis();
  holdssid.hold = true;
  
  return 1;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleSetSystem(AsyncWebServerRequest *request, uint8_t *datas) {

  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _system = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_system.success()) return 0;
  
  doAlarm = _system["hwalarm"];
  host = _system["host"].asString();
  timeZone = _system["utc"];
  language = _system["language"].asString();
  String unit = _system["unit"].asString();

  modifyconfig(eSYSTEM,{});                                      // SPEICHERN
  
  if (temp_unit != unit)  {
    temp_unit = unit;
    transform_limits();                             // Transform Limits
    modifyconfig(eCHANNEL,{});                      // Save Config
    get_Temperature();                              // Update Temperature
    #ifdef DEBUG
      DPRINTLN("[INFO]\tEinheitenwechsel");
    #endif
  }
  
  return 1;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
bool handleSetChart(AsyncWebServerRequest *request, uint8_t *datas) {

  DPRINTF("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _chart = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_chart.success()) return 0;
  
  const char* data[2];
  data[0] = _chart["thingspeak"];
  
  if (!setconfig(eTHING,data)) {
      #ifdef DEBUG
        DPRINTLN("[INFO]\tFailed to save Thingspeak config");
      #endif
    } else {
      #ifdef DEBUG
        DPRINTLN("[INFO]\tThingspeak config saved");
      #endif
    }
  
  return 1;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
String getMacAddress()  {
  uint8_t mac[6];
  char macStr[18] = { 0 };
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return  String(macStr);
}





// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
void server_setup() {

    MDNS.begin(host.c_str());  // siehe Beispiel: WiFi.hostname(host); WiFi.softAP(host);
    DPRINT("[INFO]\tOpen http://");
    DPRINT(host);
    DPRINTLN("/data to see the current temperature");

    server.on("/help",HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("https://github.com/Phantomias2006/Nemesis/blob/develop/README.md");
    }).setFilter(ON_STA_FILTER);
    

    // REQUEST: /data
    server.on("/data", HTTP_POST, [](AsyncWebServerRequest *request) { 
      handleData(request, true);
    });

    // REQUEST: /data
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) { 
      handleData(request, true);
    });

    // REQUEST: /log
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request) { 
      handleLog(request, true);
    });

    server.on("/plot", HTTP_GET, [](AsyncWebServerRequest *request) { 
      handlePlot(request, true);
    });
    
    server.on("/indexhtml", HTTP_GET, [](AsyncWebServerRequest *request) { 
      request->send(SPIFFS, "/index.html", String(), true);
    });
    
    
    // REQUEST: /settings
    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request) { 
      handleSettings(request, true);
    });

    // REQUEST: /settings
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) { 
      handleSettings(request, true);
    });
    
    // REQUEST: /networkscan
    server.on("/networkscan", HTTP_POST, [](AsyncWebServerRequest *request) { 
      handleWifiScan(request, true);
    });

    // REQUEST: /networklist
    server.on("/networklist", HTTP_POST, [](AsyncWebServerRequest *request) { 
      handleWifiResult(request, true);
    });

    // REQUEST: /networklist
    server.on("/networklist", HTTP_GET, [](AsyncWebServerRequest *request) { 
      handleWifiResult(request, true);
    });

    // REQUEST: /configreset
    server.on("/configreset", HTTP_GET, [](AsyncWebServerRequest *request) { 
      setconfig(eCHANNEL,{});
      loadconfig(eCHANNEL);
      set_Channels();
    });
    
    // REQUEST: /deletenetworkstore
    server.on("/deletenetworkstore", HTTP_POST, [](AsyncWebServerRequest *request) { 
      if (setconfig(eWIFI,{})) {  
        #ifdef DEBUG
          DPRINTLN("[INFO]\tReset wifi config");
        #endif
        request->send(200, "text/plain", "1");
      } 
      request->send(200, "text/plain", "0");
    });

    // REQUEST: /deletenetworkstore
    server.on("/deletenetworkstore", HTTP_GET, [](AsyncWebServerRequest *request) { 
      if (setconfig(eWIFI,{})) {  
        #ifdef DEBUG
          DPRINTLN("[INFO]\tReset wifi config");
        #endif
        request->send(200, "text/plain", "1");
      } 
      request->send(200, "text/plain", "0");
    });


    server.on("/alex", HTTP_GET, [](AsyncWebServerRequest *request){
      
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        const char* data[2];
        data[i] = p->value().c_str();
        setconfig(eTHING,data);
      }
      request->send(200, "text/plain", "OK");
    });

    /*  
    // REQUEST: /setnetwork
    server.on("/setnetwork", HTTP_GET, [](AsyncWebServerRequest *request) { 
      
      DPRINTLN("SSID übermittelt");
      DPRINTLN(request->method());
      DPRINTLN(request->contentType());
      DPRINTLN(request->url());
      DPRINTLN(request->host());
      int params = request->params();
      DPRINTLN(params);
      
      request->send(200, "text/plain", "OK");
    });



    server.on("/setchannels", HTTP_POST, handleSetChannels);

 
    // REQUEST: /setchannels
    server.on("/setchannels", [](AsyncWebServerRequest *request) { 
           
      if(!handleSetChannels) request->send(200, "text/plain", "0");
        request->send(200, "text/plain", "1");
      
    });
  
    

    // REQUEST: /setchannels
    server.on("/setchannels", HTTP_GET, [](AsyncWebServerRequest *request) { 
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();
              
        if(!handleSetChannels(request)) request->send(200, "text/plain", "0");
          request->send(200, "text/plain", "1");
      
    });
    server.on("/index.html",HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!handleFileRead("/index.html", request))
        request->send(404, "text/plain", "FileNotFound");
    });
    */

    // to avoid multiple requests to ESP
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html"); // gibt alles im Ordner frei
    
    // serves all SPIFFS Web file with 24hr max-age control
    //server.serveStatic("/font", SPIFFS, "/font","max-age=86400");
    //server.serveStatic("/js",   SPIFFS, "/js"  ,"max-age=86400");
    //server.serveStatic("/css",  SPIFFS, "/css" ,"max-age=86400");
    //server.serveStatic("/png",  SPIFFS, "/png" ,"max-age=86400");

    /*
    server.on("/", [](AsyncWebServerRequest *request){
      if(!handleFileRead("/", request))
        request->send(404, "text/plain", "FileNotFound");
    });
    */

    // Eventuell andere Lösung zum Auslesen des Body-Inhalts
    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/123
    // https://github.com/me-no-dev/ESPAsyncWebServer#request-variables
    
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (request->url() == "/setnetwork") {
        if (!handleSetNetwork(request, data)) request->send(200, "text/plain", "false");
          request->send(200, "text/plain", "true");
      }
      else if (request->url() =="/setchannels") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetChannels(request, data)) request->send(200, "text/plain", "false");
          request->send(200, "text/plain", "true");
      }
      else if (request->url() =="/setsystem") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetSystem(request, data)) request->send(200, "text/plain", "false");
          request->send(200, "text/plain", "true");
      }
      else if (request->url() =="/setpitmaster") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetPitmaster(request, data)) request->send(200, "text/plain", "false");
          request->send(200, "text/plain", "true");
      }
      else if (request->url() =="/setchart") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetChart(request, data)) request->send(200, "text/plain", "false");
          request->send(200, "text/plain", "true");
      } else {
        if(!index)  DPRINTF("BodyStart: %u\n", total);
        DPRINTF("%s", (const char*)data);
        if(index + len == total) DPRINTF("BodyEnd: %u\n", total);
      }
      
    });

    server.begin();
    DPRINTLN("[INFO]\tHTTP server started");
    MDNS.addService("http", "tcp", 80);

}

