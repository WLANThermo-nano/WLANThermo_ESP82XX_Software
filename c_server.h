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
    0.1.00 - 2016-12-30 initial version: Example ESP8266WebServer/FSBrowser
    
 ****************************************************/

//#include <ESP8266WebServer.h>   // https://github.com/esp8266/Arduino
//ESP8266WebServer server(80);    // declare webserver to listen on port 80
//File fsUploadFile;              // holds the current upload


AsyncWebServer server(80);        // https://github.com/me-no-dev/ESPAsyncWebServer

//AsyncWebServerRequest *holdRequest;
//unsigned long beginRequest;

const char* www_username = "admin";
const char* www_password = "admin";

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

bool handleFileRead(String path, AsyncWebServerRequest *request){
  
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path, request);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
      Serial.println(path);
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);
      if (path.endsWith(".gz"))
        response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    
    return true;
  }
  return false;
}

void handleSettings(AsyncWebServerRequest *request, bool www) {

  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server","ESP Async Web Server");
  
  String host = HOSTNAME;
  host += String(ESP.getChipId(), HEX);

  JsonObject& root = response->getRoot();
  JsonObject& _system = root.createNestedObject("system");

  _system["time"] = String(now());
  _system["utc"] = timeZone;
  _system["ap"] = APNAME;
  _system["host"] = host;
  _system["language"] = "de";
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


void handleData(AsyncWebServerRequest *request, bool www) {

  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server","ESP Async Web Server");
  
  JsonObject& root = response->getRoot();
  JsonObject& system = root.createNestedObject("system");

  system["time"] = String(now());
  system["utc"] = timeZone;
  system["soc"] = BatteryPercentage;
  system["charge"] = false;
  system["rssi"] = rssi;
  system["unit"] = temp_unit;

  JsonArray& channel = root.createNestedArray("channel");

  for (int i = 0; i < CHANNELS; i++) {
  JsonObject& data = channel.createNestedObject();
    data["number"]= i+1;
    data["name"]  = ch[i].name;
    data["typ"]   = ch[i].typ;
    data["temp"]  = ch[i].temp;
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


bool handleSetChannels(AsyncWebServerRequest *request, uint8_t *datas) {

  //  https://github.com/me-no-dev/ESPAsyncWebServer/issues/123
  Serial.print("[REQUEST]\t");
  Serial.printf("%s", (const char*)datas);
  Serial.println();

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!json.success()) {
    return 0;
  }

  int i = 0;
  JsonArray& _cha = json["channel"];
  for (JsonArray::iterator it=_cha.begin(); it!=_cha.end(); ++it) {
    int num = _cha[i]["number"];
    if (num > 0) {
      num--;          // Intern beginnt die Zählung bei 0
      String _name = _cha[i]["name"].asString();                  // KANALNAME
      if (_name.length() < 11)  ch[num].name = _name;
      byte _typ = _cha[i]["typ"];                                 // FÜHLERTYP
      if (_typ > -1 && _typ < SENSORTYPEN) ch[num].typ = _typ;  
      float _limit = _cha[i]["min"];                              // LIMITS
      if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].min = _limit;
      _limit = _cha[i]["max"];
      if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].max = _limit;
      ch[num].alarm = _cha[i]["alarm"];                           // ALARM
      ch[num].color = _cha[i]["color"].asString();                // COLOR
    }
    i++;
  }
  
  modifyconfig(eCHANNEL,{});                                      // SPEICHERN
  return 1;
}

bool handleSetPitmaster(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.print("[REQUEST]\t");
  Serial.printf("%s", (const char*)datas);
  Serial.println();
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _pitmaster = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_pitmaster.success()) return 0;
  
  pitmaster.channel = _pitmaster["channel"]; // 0
  pitmaster.typ = _pitmaster["typ"]; // ""
  pitmaster.set = _pitmaster["value"]; // 100
  pitmaster.active = _pitmaster["active"];
  return 1;
}

bool handleSetNetwork(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.print("[REQUEST]\t");
  Serial.printf("%s", (const char*)datas);
  Serial.println();
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _network = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_network.success()) return 0;
  
  const char* data[2];
  data[0] = _network["ssid"];
  data[1] = _network["password"];

  WIFI_Connect(data);

  holdssid.hold = true;
  holdssid.ssid = _network["ssid"].asString();
  holdssid.pass = _network["password"].asString();

/*
  if (!modifyconfig(eWIFI,data)) {
        #ifdef DEBUG
          Serial.println("[INFO]\tFailed to save wifi config");
        #endif
        //return 0;
      } else {
        #ifdef DEBUG
          Serial.println("[INFO]\tWifi config saved");
        #endif
        //return 1;
      }
*/
  return 1;
}

bool handleSetSystem(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.print("[REQUEST]\t");
  Serial.printf("%s", (const char*)datas);
  Serial.println();
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& _system = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
  if (!_system.success()) return 0;
  
  doAlarm = _system["hwalarm"];
  // = _system["host"];
  timeZone = _system["utc"];
  // = _system["language"];
  temp_unit = _system["unit"].asString();
  
  return 1;
}



String getMacAddress()  {
  uint8_t mac[6];
  char macStr[18] = { 0 };
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return  String(macStr);
}





void server_setup() {

    String host = HOSTNAME;
    host += String(ESP.getChipId(), HEX);
    MDNS.begin(host.c_str());  // siehe Beispiel: WiFi.hostname(host); WiFi.softAP(host);
    Serial.print("[INFO]\tOpen http://");
    Serial.print(host);
    Serial.println("/data to see the current temperature");

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
          Serial.println("[INFO]\tReset wifi config");
        #endif
        request->send(200, "text/plain", "1");
      } 
      request->send(200, "text/plain", "0");
    });

    // REQUEST: /deletenetworkstore
    server.on("/deletenetworkstore", HTTP_GET, [](AsyncWebServerRequest *request) { 
      if (setconfig(eWIFI,{})) {  
        #ifdef DEBUG
          Serial.println("[INFO]\tReset wifi config");
        #endif
        request->send(200, "text/plain", "1");
      } 
      request->send(200, "text/plain", "0");
    });

    /*  
    // REQUEST: /setnetwork
    server.on("/setnetwork", HTTP_GET, [](AsyncWebServerRequest *request) { 
      
      Serial.println("SSID übermittelt");
      Serial.println(request->method());
      Serial.println(request->contentType());
      Serial.println(request->url());
      Serial.println(request->host());
      int params = request->params();
      Serial.println(params);
      
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
        //holdRequest = request;
        //beginRequest = millis();
        if (handleSetNetwork(request, data)) {
          //request->send(200, "text/plain", "Save");
        } //else  request->send(200, "text/plain", "Fehler");
      }
      else if (request->url() =="/setchannels") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetChannels(request, data)) request->send(200, "text/plain", "0");
          request->send(200, "text/plain", "1");
      }
      else if (request->url() =="/setsystem") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetSystem(request, data)) request->send(200, "text/plain", "0");
          request->send(200, "text/plain", "1");
      }
      else if (request->url() =="/setpitmaster") { 
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();    
        if(!handleSetPitmaster(request, data)) request->send(200, "text/plain", "0");
          request->send(200, "text/plain", "1");
      } else {
        if(!index)  Serial.printf("BodyStart: %u\n", total);
        Serial.printf("%s", (const char*)data);
        if(index + len == total) Serial.printf("BodyEnd: %u\n", total);
      }
      
    });

    server.begin();
    #ifdef DEBUG
    Serial.println("[INFO]\tHTTP server started");
    #endif
    MDNS.addService("http", "tcp", 80);

}

