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
    
	  Inspired and partly taken over by Vitotai
	  https://github.com/vitotai/BrewPiLess

    HISTORY: Please refer Github History
    
 ****************************************************/

 // Eventuell andere Lösung zum Auslesen des Body-Inhalts
 // https://github.com/me-no-dev/ESPAsyncWebServer/issues/123
 // https://github.com/me-no-dev/ESPAsyncWebServer#request-variables


#define FLIST_PATH    "/list"
#define DELETE_PATH   "/rm"
#define FPUTS_PATH    "/fputs"
#define DATA_PATH     "/data"
#define SETTING_PATH  "/settings"
#define UPDATE_PATH   "/update"
#define BAD_PATH      "BAD PATH"
#define DEFAULT_INDEX_FILE  "index.html"
#define LOGLIST_PATH  "/loglist.php"
#define CHART_DATA_PATH "/chart.php"
#define NETWORK_SCAN  "/networkscan"
#define NETWORK_LIST  "/networklist"
#define NETWORK_STOP  "/stopwifi"
#define NETWORK_CLEAR "/clearwifi"
#define CONFIG_RESET  "/configreset"

#define SET_NETWORK   "/setnetwork"
#define SET_SYSTEM    "/setsystem"
#define SET_CHANNELS  "/setchannels"
#define SET_PITMASTER "/setpitmaster"
#define SET_PID       "/setpid"
#define SET_DC        "/setDC"
#define SET_IOT       "/setIoT"
#define UPDATE_CHECK  "/checkupdate"
#define UPDATE_STATUS "/updatestatus"

const char *public_list[]={
"/nano.ttf"
};

// ---------------------------------------------------------------
// WEBHANDLER
class NanoWebHandler: public AsyncWebHandler {

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  String handleSettings(AsyncWebServerRequest *request, byte www) {

    AsyncJsonResponse * response = new AsyncJsonResponse();
    response->addHeader("Server","ESP Async Web Server");
    response->addHeader("Content-Type","application/json");
  
    JsonObject& root = response->getRoot();
    JsonObject& _system = root.createNestedObject("system");

    _system["time"] =       String(now());
    _system["ap"] =         sys.apname;
    _system["host"] =       sys.host;
    _system["language"] =   sys.language;
    _system["unit"] =       temp_unit;
    _system["hwalarm"] =    sys.hwalarm;
    _system["fastmode"] =   sys.fastmode;
    _system["version"] =    FIRMWAREVERSION;
    _system["getupdate"] =  sys.getupdate;
    _system["autoupd"] =    sys.autoupdate;
    _system["hwversion"] =  String("V")+String(sys.hwversion);
  
    JsonArray& _typ = root.createNestedArray("sensors");
    for (int i = 0; i < SENSORTYPEN; i++) {
      _typ.add(ttypname[i]);
    }

    JsonArray& _pit = root.createNestedArray("pid");
    for (int i = 0; i < pidsize; i++) {
      JsonObject& _pid = _pit.createNestedObject();
      _pid["name"] =    pid[i].name;
      _pid["id"] =      pid[i].id;
      _pid["aktor"] =   pid[i].aktor;
      _pid["Kp"] =      limit_float(pid[i].Kp, -1);
      _pid["Ki"] =      limit_float(pid[i].Ki, -1);
      _pid["Kd"] =      limit_float(pid[i].Kd, -1);
      _pid["Kp_a"] =    limit_float(pid[i].Kp_a, -1);
      _pid["Ki_a"] =    limit_float(pid[i].Ki_a, -1);
      _pid["Kd_a"] =    limit_float(pid[i].Kd_a, -1);
      _pid["DCmmin"] =  pid[i].DCmin;
      _pid["DCmmax"] =  pid[i].DCmax;
    }

    JsonArray& _aktor = root.createNestedArray("aktor");
    _aktor.add("SSR");
    _aktor.add("FAN");

    JsonObject& _iot = root.createNestedObject("iot");
    _iot["TSwrite"] =   iot.TS_writeKey; 
    _iot["TShttp"] =    iot.TS_httpKey;
    _iot["TSuser"] =    iot.TS_userKey;
    _iot["TSchID"] =    iot.TS_chID;
    _iot["TSshow8"] =   iot.TS_show8;
    _iot["TSint"] =     iot.TS_int;
    _iot["TSon"] =      iot.TS_on;
    _iot["PMQhost"] =   iot.P_MQTT_HOST;
    _iot["PMQport"] =   iot.P_MQTT_PORT;
    _iot["PMQuser"] =   iot.P_MQTT_USER;
    _iot["PMQpass"] =   iot.P_MQTT_PASS;
    _iot["PMQqos"] =    iot.P_MQTT_QoS;
    _iot["PMQon"] =     iot.P_MQTT_on;
    _iot["PMQint"] =    iot.P_MQTT_int;
    //_iot["MSGservice"] = iot.TG_serv;
    _iot["TGon"]    =   iot.TG_on;
    _iot["TGtoken"] =   iot.TG_token;
    _iot["TGid"] =      iot.TG_id;
    _iot["CLon"] =      iot.CL_on;
    _iot["CLtoken"] =   iot.CL_token;
    _iot["CLint"] =     iot.CL_int;

    JsonArray& _hw = root.createNestedArray("hardware");
    _hw.add(String("V")+String(1));
    if (sys.hwversion > 1) _hw.add(String("V")+String(2));

    JsonArray& _noteservice = root.createNestedArray("notification");
    _noteservice.add("telegram");
    _noteservice.add("pushover");

    String jsonStr;
    
    if (www == 1) {
      response->setLength();
      request->send(response);
    } else if (www == 0) {
      root.printTo(Serial);
    } else  root.printTo(jsonStr);
  
    return jsonStr;
  }


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
  String handleData(AsyncWebServerRequest *request, byte www) {

    /*
    AsyncJsonResponse * response = new AsyncJsonResponse();
    response->addHeader("Server","ESP Async Web Server");
  
    JsonObject& root = response->getRoot();
    JsonObject& system = root.createNestedObject("system");

    system["time"] = String(now());
    system["soc"] = battery.percentage;
    system["charge"] = !battery.charge;
    system["rssi"] = rssi;
    system["unit"] = temp_unit;
    //system["sn"] = String(ESP.getChipId(), HEX);

    JsonArray& channel = root.createNestedArray("channel");

    for (int i = 0; i < CHANNELS; i++) {
    JsonObject& data = channel.createNestedObject();
      data["number"]= i+1;
      data["name"]  = ch[i].name;
      data["typ"]   = ch[i].typ;
      data["temp"]  = limit_float(ch[i].temp, i);
      data["min"]   = ch[i].min;
      data["max"]   = ch[i].max;
      data["alarm"] = ch[i].alarm;
      data["color"] = ch[i].color;
    }
  
    JsonObject& master = root.createNestedObject("pitmaster");

    master["channel"] = pitmaster.channel+1;
    master["pid"] = pitmaster.pid;
    master["value"] = (int)pitmaster.value;
    master["set"] = pitmaster.set;
    if (pitmaster.active)
      if (autotune.initialized)  master["typ"] = "autotune";
      else if (pitmaster.manual) master["typ"] = "manual";
      else  master["typ"] = "auto";
    else master["typ"] = "off";  

*/
    String jsonStr;
    jsonStr = cloudData();
    
    if (www == 1) {
      //response->setLength();
      //request->send(response);
      request->send(200, "application/json", jsonStr);
    } else if (www == 0) {
      //root.printTo(Serial);
      Serial.println(jsonStr);
    } //else  root.printTo(jsonStr);
  
    return jsonStr;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
  void handleWifiScan(AsyncWebServerRequest *request, bool www) {

    //dumpClients();

    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
      scantime = millis();

      if (www) request->send(200, "text/json", "OK");
      else Serial.println("OK");
    }   
  }

/*
  // ---------------------
  void handleFileList(AsyncWebServerRequest *request) {
    Dir dir = SPIFFS.openDir("/");
    String path = String();
    String output = "[";
    while(dir.next()){
      File entry = dir.openFile("r");
      if (output != "[") output += ',';
      bool isDir = false;
      output += "{\"type\":\"";
      output += (isDir)?"dir":"file";
      output += "\",\"name\":\"";
      output += String(entry.name()).substring(1);
      output += "\"}";
      entry.close();
    }
    output += "]";
    request->send(200, "text/json", output);
    output = String();
  }

  // ---------------------
  void handleFileDelete(AsyncWebServerRequest *request){
    if (request->hasParam("path", true)){
      ESP.wdtDisable(); SPIFFS.remove(request->getParam("path", true)->value()); ESP.wdtEnable(10);
      request->send(200, "", "DELETE: "+request->getParam("path", true)->value());
    } else  request->send(404);
  }

  // ---------------------
  void handleFilePuts(AsyncWebServerRequest *request){
    if(request->hasParam("path", true) && request->hasParam("content", true)){
      ESP.wdtDisable(); 
      String file=request->getParam("path", true)->value();
      File fh= SPIFFS.open(file, "w");
      if(!fh){
        request->send(500);
        return;
      }
      String c=request->getParam("content", true)->value();
      fh.print(c.c_str());
      fh.close();
      ESP.wdtEnable(10);
      request->send(200);
      DPRINTF("fputs path=%s\n",file.c_str());
      //if(file == PROFILE_FILENAME){
        //DBG_PRINTF("reload file\n");
        //brewKeeper.reloadProfile();
      //}
    } else request->send(404);
  }
*/
  /*
  // ---------------------
  bool fileExists(String path)  {
    if(SPIFFS.exists(path)) return true;
    bool dum;
    unsigned int dum2;
    if(getEmbeddedFile(path.c_str(),dum,dum2)) return true;
    if(path.endsWith(CHART_LIB_PATH) && SPIFFS.exists(CHART_LIB_PATH)) return true;
    return false;
  }
  

  // ---------------------
  void sendProgmem(AsyncWebServerRequest *request,const char* html) {
    AsyncWebServerResponse *response = request->beginResponse(String("text/html"),strlen_P(html),[=](uint8_t *buffer, size_t maxLen, size_t alreadySent) -> size_t {
      if (strlen_P(html+alreadySent)>maxLen) {
        memcpy_P((char*)buffer, html+alreadySent, maxLen);
        return maxLen;
      }
      // Ok, last chunk
      memcpy_P((char*)buffer, html+alreadySent, strlen_P(html+alreadySent));
      return strlen_P(html+alreadySent); // Return from here to end of indexhtml
    });
    response->addHeader("Cache-Control","max-age=2592000");
    request->send(response);  
  }

  // ---------------------
  void sendFile(AsyncWebServerRequest *request,String path) {
    if(SPIFFS.exists(path)){
      //request->send(SPIFFS, path);
      bool nocache=false;
      for(byte i=0;i< sizeof(nocache_list)/sizeof(const char*);i++){
        if(path.equals(nocache_list[i])){
          nocache=true;
          break;
        }
      }
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path);
      if(nocache)
        response->addHeader("Cache-Control","no-cache");
      else
        response->addHeader("Cache-Control","max-age=2592000");
      request->send(response);      
      return;
    }
    //else
    bool gzip;
    uint32_t size;
    const uint8_t* file=getEmbeddedFile(path.c_str(),gzip,size);
    if(file){
      DBG_PRINTF("using embedded file:%s\n",path.c_str());
      if(gzip){
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", file, size);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);          
      } else sendProgmem(request,(const char*)file);
    }
  }
  */

public:
  
  NanoWebHandler(void){}

  String handleData(byte www) {
    AsyncWebServerRequest *request;
    return handleData(request, www);
  }

  String handleSettings(byte www) {
    AsyncWebServerRequest *request;
    return handleSettings(request, www);
  }

  void handleWifiResult(byte www) {
    AsyncWebServerRequest *request;
    return handleWifiResult(request, www);
  }

  void handleWifiScan(byte www) {
    AsyncWebServerRequest *request;
    return handleWifiScan(request, www);
  }

  void configreset() {
    set_channels(1);
    setconfig(eCHANNEL,{});
    loadconfig(eCHANNEL);
    set_system();
    setconfig(eSYSTEM,{});
    loadconfig(eSYSTEM);
    set_pitmaster(1);
    set_pid();
    setconfig(ePIT,{});
    loadconfig(ePIT);
    set_iot(1);
    setconfig(eTHING,{});
    loadconfig(eTHING);
  }
    

  // ---------------------
  void handleRequest(AsyncWebServerRequest *request){

    if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == DATA_PATH){
      handleData(request, true);

    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == SETTING_PATH){
      handleSettings(request, true);

    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == NETWORK_SCAN){ 
      handleWifiScan(request, true);

    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == NETWORK_LIST){ 
      handleWifiResult(request, true);

    /*
    } else if (request->method() == HTTP_POST &&  request->url() == FLIST_PATH){
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();
      handleFileList(request);
      
    } else if (request->method() == HTTP_DELETE &&  request->url() == DELETE_PATH){
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();
      handleFileDelete(request);
      
    } else if (request->method() == HTTP_POST &&  request->url() == FPUTS_PATH){
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();
      handleFilePuts(request);
*/
    
    // REQUEST: /stop wifi
    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == NETWORK_STOP) { 
      isAP = 4; // Turn Wifi off with timer
      request->send(200, "text/plain", "true");
    
    // REQUEST: /clear wifi
    } else if (request->url() == NETWORK_CLEAR) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/clearwifi'>Wifi-Speicher wirklich leeren?<br><br><input type='submit' value='Ja'></form>");
      } else if (request->method() == HTTP_POST) {
        setconfig(eWIFI,{}); // clear Wifi settings
        request->send(200, "text/json", "true");
      } else request->send(500, "text/plain", BAD_PATH);

    // REQUEST: /configreset
    } else if (request->url() == CONFIG_RESET) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/configreset'>System-Speicher wirklich resetten?<br><br><input type='submit' value='Ja'></form>");
      } else if (request->method() == HTTP_POST) {
        configreset();
        request->send(200, "text/json", "true");
      } else request->send(500, "text/plain", BAD_PATH);

    // REQUEST: /update
    } else if (request->url() == UPDATE_PATH) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/update'>Version mit v eingeben: <input type='text' name='version'><br><br><input type='submit' value='Update'></form>");
      } else if (request->method() == HTTP_POST) {
        if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();
        if (request->hasParam("version", true)) { 
          ESP.wdtDisable(); 
          String version = request->getParam("version", true)->value();
          if (version.indexOf("v") == 0) sys.getupdate = version;
          else request->send(200, "text/plain", "Version unknown!");
        }
        sys.update = 1;
        ESP.wdtEnable(10);
        request->send(200, "text/json", "Do Update...");
      } else request->send(500, "text/plain", BAD_PATH);

    // REQUEST: /checkupdate
    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == UPDATE_CHECK) { 
      sys.update = -1;
      request->send(200, "text/json", "true");
    
    // REQUEST: /updatestatus
    } else if ((request->method() == HTTP_POST) &&  request->url() == UPDATE_STATUS) { 
        DPRINTLN("... in process");
        if(sys.update > 0) request->send(200, "text/plain", "true");
        request->send(200, "text/plain", "false");

    // REQUEST: File from SPIFFS
    } else if (request->method() == HTTP_GET){
      String path = request->url();
      if (path.endsWith("/")) path += DEFAULT_INDEX_FILE;
      //else if (path.endsWith(CHART_LIB_PATH)) path = CHART_LIB_PATH;
      if (request->url().equals("/")){
        //sendFile(request,path); //
        request->send(SPIFFS, path);      
        return;
      }
      bool auth = true;
      for(byte i=0;i< sizeof(public_list)/sizeof(const char*);i++){
        if(path.equals(public_list[i])){
          auth = false;
          break;
        }
      }
      if(auth && !request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      //sendFile(request,path); //
        request->send(SPIFFS, path);
    }
  }

  // --------------------- 
  bool canHandle(AsyncWebServerRequest *request){
    if (request->method() == HTTP_GET){
      if(request->url() == DATA_PATH || request->url() == SETTING_PATH 
        || request->url() == NETWORK_LIST || request->url() == NETWORK_SCAN 
        || request->url() == NETWORK_STOP || request->url() == NETWORK_CLEAR 
        || request->url() == CONFIG_RESET || request->url() == UPDATE_PATH 
        || request->url() == UPDATE_CHECK
      //|| request->url() == LOGGING_PATH
      ){
        return true;
      } else {
        // get file
        String path = request->url();
        if (path.endsWith("/")) path +=DEFAULT_INDEX_FILE;
        
        //if(fileExists(path)) return true;
        if(SPIFFS.exists(path)) return true;
        
      }
    } else if (request->method() == HTTP_DELETE && request->url() == DELETE_PATH){
      return true;
    } else if (request->method() == HTTP_POST){
      if(request->url() ==  FPUTS_PATH || request->url() == FLIST_PATH
        || request->url() == DATA_PATH || request->url() == SETTING_PATH 
        || request->url() == NETWORK_LIST || request->url() == NETWORK_SCAN
        || request->url() == NETWORK_STOP || request->url() == NETWORK_CLEAR
        || request->url() == CONFIG_RESET || request->url() == UPDATE_PATH
        || request->url() == UPDATE_CHECK || request->url() == UPDATE_STATUS
        //|| request->url() == LOGGING_PATH
        )
        return true;    
    }
    return false;
   }   
};

NanoWebHandler nanoWebHandler;


// ---------------------------------------------------------------
// WEBHANDLER
class BodyWebHandler: public AsyncWebHandler {

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setSystem(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& _system = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_system.success()) return 0;

    String unit;
  
    if (_system.containsKey("hwalarm")) sys.hwalarm = _system["hwalarm"];
    if (_system.containsKey("host")) sys.host = _system["host"].asString();
    if (_system.containsKey("language")) sys.language = _system["language"].asString();
    if (_system.containsKey("unit"))  unit = _system["unit"].asString();
    if (_system.containsKey("autoupd"))  sys.autoupdate = _system["autoupd"];
    if (_system.containsKey("fastmode")) sys.fastmode = _system["fastmode"];
    if (_system.containsKey("ap")) sys.apname = _system["ap"].asString();
    if (_system.containsKey("hwversion")) {
      String ver = _system["hwversion"].asString();
      ver.replace("V","");
      sys.hwversion = ver.toInt();
    }

    setconfig(eSYSTEM,{});                                      // SPEICHERN
  
    if (temp_unit != unit)  {
      temp_unit = unit;
      transform_limits();                             // Transform Limits
      setconfig(eCHANNEL,{});                         // Save Config
      get_Temperature();                              // Update Temperature
      DPRINTLN("[INFO]\tChange Unit");
    }
  
    return 1;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setChannels(AsyncWebServerRequest *request, uint8_t *datas) {

    //  https://github.com/me-no-dev/ESPAsyncWebServer/issues/123
  
    printRequest(datas);

    DynamicJsonBuffer jsonBuffer;
    JsonObject& _cha = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_cha.success()) return 0;
    
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
    } else return 0;
  
    setconfig(eCHANNEL,{});                                      // SPEICHERN
    return 1;
  }


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
  bool setNetwork(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
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
  bool setIoT(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& _chart = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_chart.success()) return 0;

    bool refresh = iot.CL_on;

    if (_chart.containsKey("TSwrite"))  iot.TS_writeKey = _chart["TSwrite"].asString(); 
    if (_chart.containsKey("TShttp"))   iot.TS_httpKey = _chart["TShttp"].asString(); 
    if (_chart.containsKey("TSuser"))   iot.TS_userKey = _chart["TSuser"].asString(); 
    if (_chart.containsKey("TSchID"))   iot.TS_chID = _chart["TSchID"].asString();
    if (_chart.containsKey("TSshow8"))  iot.TS_show8 = _chart["TSshow8"];
    if (_chart.containsKey("TSint"))    iot.TS_int = _chart["TSint"];
    if (_chart.containsKey("TSon"))     iot.TS_on = _chart["TSon"];
    if (_chart.containsKey("PMQhost"))  iot.P_MQTT_HOST = _chart["PMQhost"].asString(); 
    if (_chart.containsKey("PMQport"))  iot.P_MQTT_PORT = _chart["PMQport"];
    if (_chart.containsKey("PMQuser"))  iot.P_MQTT_USER = _chart["PMQuser"].asString(); 
    if (_chart.containsKey("PMQpass"))  iot.P_MQTT_PASS = _chart["PMQpass"].asString();
    if (_chart.containsKey("PMQqos"))   iot.P_MQTT_QoS = _chart["PMQqos"]; 
    if (_chart.containsKey("PMQon"))    iot.P_MQTT_on = _chart["PMQon"]; 
    if (_chart.containsKey("PMQint"))   iot.P_MQTT_int = _chart["PMQint"];
    if (_chart.containsKey("TGon"))     iot.TG_on = _chart["TGon"];
    if (_chart.containsKey("TGtoken"))  iot.TG_token = _chart["TGtoken"].asString();
    if (_chart.containsKey("TGid"))     iot.TG_id = _chart["TGid"].asString(); 
    if (_chart.containsKey("CLon"))     iot.CL_on = _chart["CLon"];
    if (_chart.containsKey("CLtoken"))  iot.CL_token = _chart["CLtoken"].asString();
    if (_chart.containsKey("CLint"))    iot.CL_int = _chart["CLint"];

    if (!refresh && iot.CL_on) lastUpdateCloud = 0; // Daten senden forcieren
  
    if (!setconfig(eTHING,{})) return 0;    
    return 1;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setPitmaster(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& _pitmaster = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_pitmaster.success()) return 0;

    String typ;
    if (_pitmaster.containsKey("typ"))
      typ = _pitmaster["typ"].asString();
    else return 0;
  
    if (_pitmaster.containsKey("channel")) {
      byte cha = _pitmaster["channel"];
      pitmaster.channel = cha - 1;
    }
    else return 0;
  
    if (_pitmaster.containsKey("pid")) pitmaster.pid = _pitmaster["pid"]; // ""
    else return 0;
    if (_pitmaster.containsKey("set")) pitmaster.set = _pitmaster["set"];
    else return 0;
  
    bool _manual = false;
    bool _autotune = false;
    if (typ == "autotune") _autotune = true;
    else if (typ == "manual") _manual = true;
    else if (typ == "auto") pitmaster.active = true;
    else  pitmaster.active = false;
    
    if (_pitmaster.containsKey("value") && _manual) {
      int _val = _pitmaster["value"];
      pitmaster.value = constrain(_val,0,100);
      pitmaster.manual = true;
      pitmaster.active = true;
      //DPRINTPLN("[INFO]\tStart Manual Pitmaster");
      return 1; // nicht speichern
    }
    else {
      pitmaster.manual = false;
    }

    if (_autotune) {
      startautotunePID(5, true, 40, 120L*60L*1000L);  // 1h Timelimit
      return 1; // nicht speichern
    } else if (autotune.initialized) {
      autotune.stop = 2;
    }
  
    if (!setconfig(ePIT,{})) return 0;
    return 1;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setPID(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonArray& json = jsonBuffer.parseArray((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!json.success()) return 0;
  
    byte id;
    byte ii;

    for (JsonArray::iterator it=json.begin(); it!=json.end(); ++it) {
      JsonObject& _pid = json[ii];
      id = _pid["id"];
      if (id >= pidsize) break;
      pid[id].name     = _pid["name"].asString();
      pid[id].aktor    = _pid["aktor"];
      pid[id].Kp       = _pid["Kp"];
      pid[id].Ki       = _pid["Ki"];
      pid[id].Kd       = _pid["Kd"];
      pid[id].Kp_a     = _pid["Kp_a"];
      pid[id].Ki_a     = _pid["Ki_a"];
      pid[id].Kd_a     = _pid["Kd_a"];
      //pid[id].reversal = _pid["reversal"];
      pid[id].DCmin    = _pid["DCmmin"];
      pid[id].DCmax    = _pid["DCmmax"];
      ii++;
    }
  
    if (!setconfig(ePIT,{})) return 0;
    return 1;
  }


public:
  
  BodyWebHandler(void){}

  bool setNetwork(uint8_t *datas) {
    AsyncWebServerRequest *request;
    setNetwork(request, datas);
  }

  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    
    if (request->url() == SET_NETWORK) {
      if (!setNetwork(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
    
    } else if (request->url() == SET_CHANNELS) { 
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      if(!setChannels(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
    
    } else if (request->url() == SET_SYSTEM) {
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      if(!setSystem(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
 
    } else if (request->url() == SET_PITMASTER) { 
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      if(!setPitmaster(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
    
    } else if (request->url() == SET_PID) { 
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      if(!setPID(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
      
    } else if (request->url() == SET_IOT) { 
      if(!request->authenticate(www_username, www_password))
        return request->requestAuthentication();    
      if(!setIoT(request, data)) request->send(200, "text/plain", "false");
        request->send(200, "text/plain", "true");
    }  
  }

  bool canHandle(AsyncWebServerRequest *request){
    if (request->method() == HTTP_GET) return false; 
    if (request->url() == SET_NETWORK || request->url() == SET_CHANNELS
      || request->url() == SET_SYSTEM || request->url() == SET_PITMASTER
      || request->url() == SET_PID || request->url() == SET_IOT
      ) return true;
    return false;
  }

};

BodyWebHandler bodyWebHandler;

