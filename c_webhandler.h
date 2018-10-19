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
#define SET_PUSH      "/setPush"
#define UPDATE_CHECK  "/checkupdate"
#define UPDATE_STATUS "/updatestatus"
#define DC_STATUS     "/dcstatus"

#define APPLICATIONJSON "application/json"
#define TEXTPLAIN "text/plain"
#define TEXTON "aktiviert"
#define TEXTOFF "deaktiviert"
#define TEXTTRUE "true"
#define TEXTFALSE "false"
#define TEXTADD "Add"

const char *public_list[]={
"/nano.ttf"
};

// ---------------------------------------------------------------
// WEBHANDLER
class NanoWebHandler: public AsyncWebHandler {

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  void handleSettings(AsyncWebServerRequest *request) {
    
    String jsonStr;
    jsonStr = cloudSettings();
    
    request->send(200, APPLICATIONJSON, jsonStr);
  }


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
  void handleData(AsyncWebServerRequest *request) {

    String jsonStr;
    jsonStr = cloudData(false);
    
    request->send(200, APPLICATIONJSON, jsonStr);
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
        json["Scantime"]  = millis()-wifi.scantime;
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

    // kein Scan zu Systemstart sonst keine Reconnection nach Systemstart

    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);        // true = scan async
      wifi.scantime = millis();

      if (www) request->send(200, TEXTPLAIN, "OK");
      //else Serial.println("OK");
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
    loadconfig(eCHANNEL,0);
    set_system();
    setconfig(eSYSTEM,{});
    loadconfig(eSYSTEM,0);
    set_pitmaster(1);
    set_pid(0);
    setconfig(ePIT,{});
    loadconfig(ePIT,0);
    set_iot(1);
    set_push();
    setconfig(eTHING,{});
    loadconfig(eTHING,0);
  }
    

  // ---------------------
  void handleRequest(AsyncWebServerRequest *request){

    if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == DATA_PATH){
      handleData(request);

    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == SETTING_PATH){
      handleSettings(request);

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
      wifi.mode = 4; // Turn Wifi off with timer
      request->send(200, TEXTPLAIN, TEXTTRUE);
    
    // REQUEST: /clear wifi
    } else if (request->url() == NETWORK_CLEAR) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/clearwifi'>Wifi-Speicher wirklich leeren?<br><br><input type='submit' value='Ja'></form>");
      } else if (request->method() == HTTP_POST) {
        setconfig(eWIFI,{}); // clear Wifi settings
        sys.restartnow = true;
        wifi.mode = 5;
        request->send(200, TEXTPLAIN, TEXTTRUE);
      } else request->send(500, TEXTPLAIN, BAD_PATH);

    // REQUEST: /configreset
    } else if (request->url() == CONFIG_RESET) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/configreset'>System-Speicher wirklich resetten?<br><br><input type='submit' value='Ja'></form>");
      } else if (request->method() == HTTP_POST) {
        configreset();
        request->send(200, TEXTPLAIN, TEXTTRUE);
      } else request->send(500, TEXTPLAIN, BAD_PATH);

    // REQUEST: /update
    } else if (request->url() == UPDATE_PATH) {
      if (request->method() == HTTP_GET) {
        request->send(200, "text/html", "<form method='POST' action='/update'>Version mit v eingeben: <input type='text' name='version'><br><br><input type='submit' value='Update'></form>");
      } else if (request->method() == HTTP_POST) {
        if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
          return request->requestAuthentication();
        if (request->hasParam("version", true)) { 
          ESP.wdtDisable(); 
          // use getParam(xxx, true) for form-data parameters in POST request header
          String version = request->getParam("version", true)->value();
          if (version.indexOf("v") == 0) sys.getupdate = version;
          else request->send(200, TEXTPLAIN, "Version unknown!");
        }
        sys.update = 1;
        ESP.wdtEnable(10);
        request->send(200, TEXTPLAIN, "Do Update...");
      } else request->send(500, TEXTPLAIN, BAD_PATH);

    // REQUEST: /checkupdate
    } else if ((request->method() == HTTP_POST || request->method() == HTTP_GET) &&  request->url() == UPDATE_CHECK) { 
      sys.update = -1;
      request->send(200, TEXTPLAIN, TEXTTRUE);
    
    // REQUEST: /updatestatus
    } else if ((request->method() == HTTP_POST) &&  request->url() == UPDATE_STATUS) { 
        DPRINTLN("... in process");
        if(sys.update > 0) request->send(200, TEXTPLAIN, TEXTTRUE);
        request->send(200, TEXTPLAIN, TEXTFALSE);

    // REQUEST: /dcstatus
    } else if ((request->method() == HTTP_POST) &&  request->url() == DC_STATUS) { 
        if (pitMaster[0].active == DUTYCYCLE || pitMaster[1].active == DUTYCYCLE) request->send(200, TEXTPLAIN, TEXTTRUE);
        else request->send(200, TEXTPLAIN, TEXTFALSE);

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
      if(auth && !request->authenticate(sys.www_username, sys.www_password.c_str()))
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
        || request->url() == DC_STATUS  //|| request->url() == LOGGING_PATH
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
  int checkStringLength(String tex) {
    int index = tex.length();
    while (tex.lastIndexOf(195) != -1) {
      tex.remove(tex.lastIndexOf(195),1);
      index--;
    }
    return index;
  }

  String checkString(String tex) {  
    tex.replace("&amp;","&");   // &
    tex.replace("&lt;","<");   // <
    tex.replace("&gt;",">");   // >
    return tex;
  }

  
  void servoV2(bool dc = false) {  
      
    if (dc) {
      pitMaster[0].io = PITMASTER2;         // SERVO SWITCH
      pitMaster[1].io = PITMASTER1;
      DC_start(dc, FAN, 500, 1);          // dc = 1 damit kein Softstart
      //bbq[1].DC_start(dc, SUPPLY, 500);   // Oberer Grenzwert, SUPPLY, 50%
      return;
    }
    Serial.println("V2-Aktorik");
    if (pid[pitMaster[0].pid].aktor == SERVO && pitMaster[0].active > 0 && pitMaster[1].active != VOLTAGE) {
      pitMaster[0].io = PITMASTER2;         // SERVO SWITCH
      pitMaster[1].io = PITMASTER1;
      pitMaster[1].value = 50;        // 6V
      pitMaster[1].active = VOLTAGE;
      //Serial.println("IO-Switch");
    
    } else if ((pid[pitMaster[0].pid].aktor != SERVO || pitMaster[0].active == PITOFF)  && pitMaster[1].active == VOLTAGE) {
      // kein IO-Wechsel, nur in disableHeater
      pitMaster[1].active = PITOFF;
      //Serial.println("abschalten");
    }
  }

  void damperV2() {  
    
    if (pid[pitMaster[0].pid].aktor == DAMPER) {

      if (pid[2].aktor == SERVO) {   
        pitMaster[1].pid = 2;
        pitMaster[1].channel = pitMaster[0].channel;
        pitMaster[1].set = pitMaster[0].set;
        pitMaster[1].active = pitMaster[0].active;
        pitMaster[1].value = pitMaster[0].value;  // Manual             
      } // FAN trotzdem laufen lassen? Oder Speichern abbrechen?

    } else if (pid[pitMaster[0].pid].aktor != DAMPER && (pitMaster[1].active != VOLTAGE)) {  // bbq[1].getStatus() != PITOFF
      pitMaster[1].active = PITOFF;
    }
  }


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setSystem(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& _system = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_system.success()) return 0;

    String unit, _name;
  
    if (_system.containsKey("language"))  sys.language   = _system["language"].asString();
    if (_system.containsKey("unit"))      unit = _system["unit"].asString();
    if (_system.containsKey("autoupd"))   sys.autoupdate = _system["autoupd"];
    if (_system.containsKey("fastmode"))  sys.fastmode   = _system["fastmode"];

    if (_system.containsKey("host")) {
      _name = _system["host"].asString();
      if (checkStringLength(_name) < 14)  sys.host = _name;
    }

    if (_system.containsKey("ap")) {
      _name = _system["ap"].asString();
      if (checkStringLength(_name) < 14)  sys.apname = _name;
    }
    
    if (_system.containsKey("hwversion")) {
      _name = _system["hwversion"].asString();
      _name.replace("V","");
      if (_name.indexOf('+') > 0) sys.hwversion = 2;    // V1+
      else  sys.hwversion = _name.toInt();
    }

    setconfig(eSYSTEM,{});                                      // SPEICHERN
  
    if (sys.unit != unit)  {
      sys.unit = unit;
      transform_limits();                             // Transform Limits
      setconfig(eCHANNEL,{});                         // Save Config
      get_Temperature();                              // Update Temperature
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
    if (num > 0) {     // Feld vorhanden
      num--;          // Intern beginnt die Zählung bei 0
      String _name;
      if (_cha.containsKey("name")) {
        _name = _cha["name"].asString();   // KANALNAME
        if (checkStringLength(_name) < 11)  ch[num].name = _name;
      }

      byte _typ;
      if (_cha.containsKey("typ")) {
        _typ = _cha["typ"];                 // FÜHLERTYP
        if (_typ > -1 && _typ < SENSORTYPEN) ch[num].typ = _typ; 
      }
       
      float _limit;
      if (_cha.containsKey("min")) {
        _limit = _cha["min"];               // LIMIT
        if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].min = _limit;
      }
      
      if (_cha.containsKey("max")) {
        _limit = _cha["max"];               // LIMIT
        if (_limit > LIMITUNTERGRENZE && _limit < LIMITOBERGRENZE) ch[num].max = _limit;
      }
        
      if (_cha.containsKey("alarm"))  ch[num].alarm = _cha["alarm"];   // ALARM
      if (_cha.containsKey("color"))  ch[num].color = _cha["color"].asString();   // COLOR
      
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
    holdssid.ssid = checkString(_network["ssid"].asString());
    if (!_network.containsKey("password")) return 0;
    holdssid.pass = _network["password"].asString();
    holdssid.connect = millis();
    holdssid.hold = 1;
  
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
    if (_chart.containsKey("TShttp"))   iot.TS_httpKey  = _chart["TShttp"].asString(); 
    if (_chart.containsKey("TSuser"))   iot.TS_userKey  = _chart["TSuser"].asString(); 
    if (_chart.containsKey("TSchID"))   iot.TS_chID     = _chart["TSchID"].asString();
    if (_chart.containsKey("TSshow8"))  iot.TS_show8    = _chart["TSshow8"];
    if (_chart.containsKey("TSint"))    iot.TS_int      = _chart["TSint"];
    if (_chart.containsKey("TSon"))     iot.TS_on       = _chart["TSon"];
    
    if (_chart.containsKey("PMQhost"))  iot.P_MQTT_HOST = _chart["PMQhost"].asString(); 
    if (_chart.containsKey("PMQport"))  iot.P_MQTT_PORT = _chart["PMQport"];
    if (_chart.containsKey("PMQuser"))  iot.P_MQTT_USER = _chart["PMQuser"].asString(); 
    if (_chart.containsKey("PMQpass"))  iot.P_MQTT_PASS = _chart["PMQpass"].asString();
    if (_chart.containsKey("PMQqos"))   iot.P_MQTT_QoS  = _chart["PMQqos"]; 
    if (_chart.containsKey("PMQon"))    iot.P_MQTT_on   = _chart["PMQon"]; 
    if (_chart.containsKey("PMQint"))   iot.P_MQTT_int  = _chart["PMQint"];
    
    if (_chart.containsKey("CLon"))     iot.CL_on       = _chart["CLon"];
    if (_chart.containsKey("CLtoken"))  iot.CL_token    = _chart["CLtoken"].asString();
    if (_chart.containsKey("CLint"))    iot.CL_int      = _chart["CLint"];

    if (!refresh && iot.CL_on) lastUpdateCloud = 1; // Daten senden forcieren
  
    if (!setconfig(eTHING,{})) return 0;    
    return 1;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
  bool setPush(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonObject& _push = jsonBuffer.parseObject((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!_push.success()) return 0;

    if (_push.containsKey("on"))      pushd.on       = (byte)_push["on"];
    if (_push.containsKey("token"))   pushd.token    = _push["token"].asString();
    if (_push.containsKey("id"))      pushd.id       = _push["id"].asString(); 
    if (_push.containsKey("repeat"))  pushd.repeat   = _push["repeat"];
    if (_push.containsKey("service")) pushd.service  = _push["service"];

    if (pushd.on == 2) notification.type = 1;    // Verbindungstest
    else {
      if (!setconfig(ePUSH,{})) return 0;     // nicht bei Verbindungstest speichern
    } 
    return 1;
  }

  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  bool setPitmaster(AsyncWebServerRequest *request, uint8_t *datas) {

    printRequest(datas);
  
    DynamicJsonBuffer jsonBuffer;
    JsonArray& json = jsonBuffer.parseArray((const char*)datas);   //https://github.com/esp8266/Arduino/issues/1321
    if (!json.success()) return 0;
  
    byte id, ii = 0;

    for (JsonArray::iterator it=json.begin(); it!=json.end(); ++it) {
       
      JsonObject& _pitmaster = json[ii];
      
      if (_pitmaster.containsKey("id")) id = _pitmaster["id"];
      else break;
      if (id >= PITMASTERSIZE) break;

      String typ;
      if (_pitmaster.containsKey("typ"))
        typ = _pitmaster["typ"].asString();
      else return 0;
  
      if (_pitmaster.containsKey("channel")) {
        byte cha = _pitmaster["channel"];
        pitMaster[id].channel = cha - 1;

        open_lid_init(); // Speicher zurücksetzen
      }
      else return 0;
  
      if (_pitmaster.containsKey("pid")) {
        byte temppid = _pitmaster["pid"];
        if (temppid != pitMaster[ii].pid) {
          pitMaster[id].disabled = false;
          disableHeater(id);
          //Serial.println("PID-Wechsel");
        }
        pitMaster[ii].pid = temppid;
      
      } else return 0;
      if (_pitmaster.containsKey("set")) pitMaster[ii].set = _pitmaster["set"];
      else return 0;
  
      bool _manual = false;
      bool _autotune = false;
    
      if (typ == "autotune") _autotune = true;
      else if (typ == "manual") _manual = true;
      else if (typ == "auto") pitMaster[id].active = AUTO;
      else  pitMaster[id].active = PITOFF;
    
      if (_pitmaster.containsKey("value") && _manual) {
        int _val = _pitmaster["value"];
        pitMaster[id].value = constrain(_val,0,100);
        pitMaster[id].active = MANUAL;
        //return 1; // nicht speichern
      }

      if (_autotune && id == 0) {
        startautotunePID(5, true, 40, 120L*60L*1000L, id);  // 1h Timelimit
        return 1; // nicht speichern
      } else if (autotune.initialized) {    // Autotune was still in action
        autotune.stop = 2;
      }
      
      ii++;
    }

    // Spezial-Funktionen
    if (sys.hwversion > 1) {
      servoV2();
      damperV2();
    } else {
      pitMaster[1].active = PITOFF;
      pitMaster[0].io = PITMASTER1;     // Zurücksetzen
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
  
    byte id = 0, ii = 0;
    float val;

    for (JsonArray::iterator it=json.begin(); it!=json.end(); ++it) {
      JsonObject& _pid = json[ii];
      if (_pid.containsKey("id")) id = _pid["id"];
      else break;
      if (id >= pidsize) break;
      if (_pid.containsKey("name"))   pid[id].name     = _pid["name"].asString();
      if (_pid.containsKey("aktor"))  pid[id].aktor    = _pid["aktor"];
      if (_pid.containsKey("Kp"))     pid[id].Kp       = _pid["Kp"];
      if (_pid.containsKey("Ki"))     pid[id].Ki       = _pid["Ki"];
      if (_pid.containsKey("Kd"))     pid[id].Kd       = _pid["Kd"];
      if (_pid.containsKey("Kp_a"))   pid[id].Kp_a     = _pid["Kp_a"];
      if (_pid.containsKey("Ki_a"))   pid[id].Ki_a     = _pid["Ki_a"];
      if (_pid.containsKey("Kd_a"))   pid[id].Kd_a     = _pid["Kd_a"];
      if (_pid.containsKey("DCmmin")) {
        val = _pid["DCmmin"];
        if (val >= SERVOPULSMIN && val <= SERVOPULSMAX && pid[id].aktor == SERVO) {
          pid[id].DCmin = getDC(val*10)/10.0;    
        } else pid[id].DCmin = constrain(val*10,0,1000)/10.0;    // 1. Nachkommastelle
      }
      if (_pid.containsKey("DCmmax")) {
        val = _pid["DCmmax"];
        if (val >= SERVOPULSMIN && val <= SERVOPULSMAX && pid[id].aktor == SERVO) {
          pid[id].DCmax = getDC(val*10)/10.0;    
        } else pid[id].DCmax = constrain(val*10,0,1000)/10.0;    // 1. Nachkommastelle
      }    
      if (_pid.containsKey("opl"))   pid[id].opl     = _pid["opl"];
      ii++;
    }
  
    if (!setconfig(ePIT,{})) return 0;
    return 1;
  }


public:
  
  BodyWebHandler(void){}

  //
  void setservoV2(bool dc) {
    servoV2(dc);
  }

  void setdamperV2() {
    damperV2();
  }

  // {"ssid":"xxx","password":"xxx"}
  bool setNetwork(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setNetwork(request, datas);
  }

  // {"number":1,"name":"Kanal 1","typ":0,"temp":24.50,"min":10.00,"max":35.00,"alarm":false,"color":"#0C4C88"}
  bool setChannels(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setChannels(request, datas);
  }

  // {"ap":"NANO-AP","host":"NANO-82e0b3","language":"de","unit":"C","hwalarm":false,"fastmode":false,"autoupd":true,"hwversion":"V1"}
  bool setSystem(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setSystem(request, datas);
  }

  // {"channel":1,"pid":0,"value":0,"set":50.00,"typ":"off"}
  bool setPitmaster(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setPitmaster(request, datas);
  }

  bool setPID(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setPID(request, datas);
  }

  // {"TSwrite":"","TShttp":"","TSuser":"","TSchID":"","TSshow8":false,"TSint":30,"TSon":false,"PMQhost":"192.168.2.1","PMQport":1883,"PMQuser":"","PMQpass":"","PMQqos":0,"PMQon":false,"PMQint":30,"CLon":true,"CLtoken":"82e0b30c6486bede","CLint":30}
  bool setIoT(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setIoT(request, datas);
  }

  // {"on":true,"token":"","id":"","repeat":1,"service":1}
  bool setPush(uint8_t *datas) {
    AsyncWebServerRequest *request;
    return setPush(request, datas);
  }

  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    
    if (request->url() == SET_NETWORK) {
      if (!setNetwork(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
    
    } else if (request->url() == SET_CHANNELS) { 
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setChannels(request,data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
    
    } else if (request->url() == SET_SYSTEM) {
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setSystem(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
 
    } else if (request->url() == SET_PITMASTER) { 
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setPitmaster(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
    
    } else if (request->url() == SET_PID) { 
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setPID(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
      
    } else if (request->url() == SET_IOT) { 
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setIoT(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);

    } else if (request->url() == SET_PUSH) { 
      if(!request->authenticate(sys.www_username, sys.www_password.c_str()))
        return request->requestAuthentication();    
      if(!setPush(request, data)) request->send(200, TEXTPLAIN, TEXTFALSE);
        request->send(200, TEXTPLAIN, TEXTTRUE);
    }
  }

  bool canHandle(AsyncWebServerRequest *request){
    if (request->method() == HTTP_GET) return false; 
    if (request->url() == SET_NETWORK || request->url() == SET_CHANNELS
      || request->url() == SET_SYSTEM || request->url() == SET_PITMASTER
      || request->url() == SET_PID || request->url() == SET_IOT
      || request->url() == SET_PUSH
      ) return true;
    return false;
  }

};

BodyWebHandler bodyWebHandler;

